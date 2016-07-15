/*
 * Copyright 2016 Steven Franzen <sfranzen85@gmail.com>
 *
 * This file is part of KTuner.
 *
 * KTuner is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * KTuner is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * KTuner. If not, see <http://www.gnu.org/licenses/>.
 */

#include "analyzer.h"
#include "ktunerconfig.h"

#include <QtMath>
#include <QDebug>

#include <limits.h>

const Spectrum Analyzer::NullResult = Spectrum() << Tone(0, 0);

Analyzer::Analyzer(QObject* parent)
    : QObject(parent)
    , m_numNoiseSegments(10)
    , m_plan(Q_NULLPTR)
    , m_currentSpectrum(0)
{
    init();
    connect(KTunerConfig::self(), &KTunerConfig::configChanged, this, &Analyzer::init);
    setState(Ready);
}

void Analyzer::init()
{
    setState(Loading);
    m_sampleSize = KTunerConfig::segmentLength();
    m_numSpectra = KTunerConfig::numSpectra();
    m_currentSpectrum %= m_numSpectra;

    m_outputSize = 0.5 * m_sampleSize + 1;
    m_window.resize(m_sampleSize);
    calculateWindow();
    m_input.fill(0, m_sampleSize);
    m_output.fill(0, m_outputSize);
    m_spectrum.fill(0, m_outputSize);
    m_noiseSpectrum.fill(0, m_outputSize);
    m_filterMode = true;
    m_spectrumHistory.fill(m_spectrum, m_numSpectra);
    
    if (m_plan)
        fftw_destroy_plan(m_plan);
    // FFTW and C++(99) complex types are binary compatible
    m_plan = fftw_plan_dft_r2c_1d(m_sampleSize,
                                  m_input.data(),
                                  reinterpret_cast<fftw_complex*>(m_output.data()),
                                  FFTW_MEASURE
    );
    setState(Ready);
}

Analyzer::~Analyzer()
{
    fftw_destroy_plan(m_plan);
    fftw_cleanup();
}

void Analyzer::doAnalysis(QByteArray input, const QAudioFormat &format)
{
    if (m_state != Ready)
        return;

    if (m_filterMode)
        setState(CalibratingFilter);
    else
        setState(Processing);

    // Process the bytearray into m_input
    if (m_currentFormat != format) m_currentFormat = format;
    preProcess(input);

    // Obtain frequency information
    fftw_execute(m_plan);    
    for (quint32 i = 1; i < m_outputSize; ++i) {
        m_spectrum[i].frequency = qreal(i) * format.sampleRate() / m_sampleSize;
        m_spectrum[i].amplitude = qPow(std::abs(m_output.at(i)), 2) /  m_sampleSize;
    }
    
    processFilter();
    processSpectrum();

    emit done(determineFundamental(), m_spectrum);
    setState(Ready);
}

Spectrum Analyzer::determineFundamental() const
{
    const Spectrum peaks = interpolatePeaks(10);

    if (peaks.isEmpty()) {
        return NullResult;
    }

    // Check how well each peak frequency divides the others. The fundamental
    // frequency should have the most near-integer ratios, "near-integer"
    // meaning within half a semitone of the nearest integer.
    const static qreal range = 1.0 / 24;
    const static qreal interval = qPow(2, range) - qPow(2, -range);
    Spectrum candidates;
    Spectrum harmonics;
    candidates.reserve(10);
    harmonics.reserve(10);
    qreal maxPower = 0;
    for (auto i = peaks.constBegin(); i < peaks.constEnd(); ++i) {
        qreal currentPower = 0;
        candidates.clear();
        for (auto j = i; j < peaks.constEnd(); ++j) {
            qreal ratio = j->frequency / i->frequency;
            int nearestInt = qRound(ratio);
            qreal remainder = qAbs(ratio - nearestInt);
            if (nearestInt > 0 && remainder < nearestInt * interval) {
                currentPower += j->amplitude;
                candidates.append(*j);
            }
        }
        if (currentPower > maxPower) {
            maxPower = currentPower;
            harmonics.swap(candidates);
        }
    }
    return harmonics;
}

Spectrum Analyzer::interpolatePeaks(int numPeaks) const
{
    numPeaks = qMax(1, numPeaks);
    Spectrum peaks;
    peaks.reserve(numPeaks);

    // Compute central differences
    QList<qreal> derivative;
    derivative.reserve(m_spectrum.size());
    for (auto s = m_spectrum.constBegin() + 2; s < m_spectrum.constEnd() - 1; ++s) {
        qreal dy = (s + 1)->amplitude - (s - 1)->amplitude;
        qreal dx = 2 * ((s + 1)->frequency - s->frequency);
        derivative.append(dy / dx);
    }

    // Smooth
    for (int i = 0; i < 5; ++i)
        for (auto d = derivative.begin() + 1; d < derivative.end() - 1; ++d) {
            *d = (*(d - 1) + *d + *(d + 1)) / 3;
        }

    // Compute conditions for zero crossings
    qreal averagePower = 0;
    for (auto s = m_spectrum.constBegin() + 1; s < m_spectrum.constEnd() - 1; ++s)
        averagePower += s->amplitude;
    averagePower /= m_spectrum.size() - 2;
    const qreal minPower = averagePower / 5;
    const qreal minSlope = -0.2;

    // Locate zero crossings
    int n = 0;
    QList<int> binPeaks;
    for (int i = 1; n < numPeaks && i < derivative.size() - 1; ++i) {
        if ( derivative.at(i) < minSlope && derivative.at(i - 1) > 0 && derivative.at(i + 1) < 0 &&
            m_spectrum.at(2 + i).frequency > 40 && m_spectrum.at(2 + i).amplitude > minPower)
        {
            binPeaks.append(2 + i);
            ++n;
        }
    }

    for (int i = 0; i < binPeaks.size(); ++i) {
        // Interpolate
        int k = binPeaks.at(i);
        qreal peak = k;
        if (k > 1 && k < m_spectrum.size() - 1) {
            qreal delta;
            switch(KTunerConfig::windowFunction()) {
            case KTunerConfig::NoWindow:
                // Interpolate using the complex coefficients
                delta = -std::real((m_output[k+1] - m_output[k-1]) /
                    (2.0 * m_output[k] - m_output[k+1] - m_output[k-1]));
                break;
            default:
                // This interpolation works better with window functions
                delta = std::real(0.55 * (m_output[k-1] - m_output[k+1]) /
                    (m_output[k-1] + 2.0*m_output[k] + m_output[k+1]));
                break;
            }
            peak += delta;
        }
        peaks.append(Tone(peak * m_currentFormat.sampleRate() / m_sampleSize, m_spectrum.at(k).amplitude));
    }
    return peaks;
}

void Analyzer::calculateWindow()
{
    for (quint32 i = 0; i < m_sampleSize; ++i) {
        switch(KTunerConfig::windowFunction()) {
        case KTunerConfig::NoWindow:
            m_window[i] = 1.0;
            break;
        case KTunerConfig::HannWindow:
            m_window[i] = 0.5 * (1 - qCos((2 * M_PI * i) / (m_sampleSize - 1)));
            break;
        case KTunerConfig::GaussianWindow:
            m_window[i] = qExp(-0.5 * qPow( (i - 0.5 * (m_sampleSize - 1)) /
                                            (0.4 * 0.5 * (m_sampleSize - 1)), 2));
            break;
        default:
            Q_UNREACHABLE();
        }
    }
}

void Analyzer::preProcess(QByteArray input)
{
    // If not enough data is provided, pad with zeros
    const int difference = m_sampleSize * m_currentFormat.sampleSize() / 8 - input.size();
    if (difference > 0) {
        qDebug() << "Analyzer input too short, padding with zeros.";
        input.append(QByteArray(difference, 0));
    }
    
    // Extract and scale the audio samples from the buffer
    const char *ptr = input.constData();
    for (quint32 i = 0; i < m_sampleSize; ++i) {
        const qint16 sample = *reinterpret_cast<const qint16*>(ptr);
        m_input[i] = qreal(sample) / std::numeric_limits<qint16>::max();
        ptr += m_currentFormat.sampleSize() / 8;
    }

    // Find a simple least squares fit y = ax + b to the scaled input
    const qreal xMean = 0.5 * m_sampleSize;
    const qreal sum = std::accumulate(m_input.constBegin(), m_input.constEnd(), 0.0);
    const qreal yMean = sum / m_sampleSize;
    qreal covXY = 0, varX = 0; // Cross-covariance and variance
    for (quint32 x = 0; x < m_sampleSize; ++x) {
        covXY += (x - xMean) * (m_input.at(x) - yMean);
        varX += qPow(x - xMean, 2.0);
    }
    const qreal a = covXY / varX;
    const qreal b = yMean - a * xMean;

    // Subtract this fit and apply the window function
    for (quint32 x = 0; x < m_sampleSize; ++x) {
        m_input[x] = m_window.at(x) * (m_input.at(x) - (a * x + b));
    }
}

void Analyzer::processFilter()
{
    if (!m_filterMode) return;
    static quint32 filterPass = 0;
    Spectrum::iterator i;
    Spectrum::const_iterator j;
    if (filterPass == 0) {
        m_noiseSpectrum.fill(0);
        for (i = m_noiseSpectrum.begin(), j = m_spectrum.constBegin(); i < m_noiseSpectrum.end(); ++i, ++j)
            i->frequency = j->frequency;
    }
    if (filterPass < m_numNoiseSegments) {
        filterPass++;
        for (i = m_noiseSpectrum.begin(), j = m_spectrum.constBegin(); i < m_noiseSpectrum.end(); ++i, ++j)
            i->amplitude += j->amplitude / m_numNoiseSegments;
    } else {
        filterPass = 0;
        m_filterMode = false;
    }
}

void Analyzer::processSpectrum()
{
    m_spectrumHistory[m_currentSpectrum].swap(m_spectrum);
    m_currentSpectrum = (m_currentSpectrum + 1) % m_numSpectra;

    for (int i = 0; i < m_spectrum.size(); ++i) {
        qreal sum = 0;
        for (auto s = m_spectrumHistory.constBegin(); s < m_spectrumHistory.constEnd(); ++s) {
            sum += s->at(i).amplitude;
        }
        m_spectrum[i].amplitude = qMax(0.0, sum / m_numSpectra - m_noiseSpectrum.at(i).amplitude);
    }
}

void Analyzer::computeNoiseFilter()
{
    m_filterMode = true;
}

void Analyzer::removeNoiseFilter()
{
    m_noiseSpectrum.fill(0);
}

void Analyzer::setState(Analyzer::State newState)
{
    if (m_state != newState) {
        m_state = newState;
        emit stateChanged(newState);
    }
}

Analyzer::State Analyzer::state() const
{
    return m_state;
}

#include "analyzer.moc"
