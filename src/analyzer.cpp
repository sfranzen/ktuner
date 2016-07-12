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

Analyzer::Analyzer(QObject* parent)
    : QObject(parent)
    , m_plan(Q_NULLPTR)
    , m_currentSpectrum(0)
{
    init();
    connect(KTunerConfig::self(), &KTunerConfig::configChanged, this, &Analyzer::init);
    m_ready = true;
}

void Analyzer::init()
{
    m_sampleSize = KTunerConfig::segmentLength();
    m_numSpectra = KTunerConfig::numSpectra();
    m_currentSpectrum %= m_numSpectra;

    m_outputSize = 0.5 * m_sampleSize + 1;
    m_window.resize(m_sampleSize);
    calculateWindow();
    m_input.fill(0, m_sampleSize);
    m_output.fill(0, m_outputSize);
    m_spectrum.fill(0, m_outputSize);
    m_spectrumHistory.fill(m_spectrum, m_numSpectra);
    
    if (m_plan)
        fftw_destroy_plan(m_plan);
    // FFTW and C++(99) complex types are binary compatible
    m_plan = fftw_plan_dft_r2c_1d(m_sampleSize,
                                  m_input.data(),
                                  reinterpret_cast<fftw_complex*>(m_output.data()),
                                  FFTW_MEASURE
    );
}

Analyzer::~Analyzer()
{
    fftw_destroy_plan(m_plan);
    fftw_cleanup();
}

bool Analyzer::isReady() const
{
    return m_ready;
}

void Analyzer::doAnalysis(QByteArray input, const QAudioFormat &format)
{
    m_ready = false;

    // Process the bytearray into m_input
    if (m_currentFormat != format) m_currentFormat = format;
    preProcess(input);

    // Obtain frequency information
    fftw_execute(m_plan);    
    for (quint32 i = 1; i < m_outputSize; ++i) {
        m_spectrum[i].frequency = qreal(i) * format.sampleRate() / m_sampleSize;
        m_spectrum[i].amplitude = qPow(std::abs(m_output.at(i)), 2) /  m_sampleSize;
    }
    
    m_spectrumHistory[m_currentSpectrum] = m_spectrum;
    m_currentSpectrum = (m_currentSpectrum + 1) % m_numSpectra;
    averageSpectra();
    
    const qreal estimate = determineFundamental(m_spectrum);
    emit done(estimate, m_spectrum);
    m_ready = true;
}

qreal Analyzer::determineFundamental(Spectrum spectrum) const
{
    const QList<qreal> peaks = interpolatePeaks(spectrum, 10);
    if (peaks.isEmpty())
        return 0;

    // Check how well each peak frequency divides the others. The fundamental
    // frequency should have the most near-integer ratios, "near-integer"
    // meaning within half a semitone of the nearest integer.
    const static qreal range = 1.0 / 24;
    const static qreal interval = qPow(2, range) - qPow(2, -range);
    qreal candidateFrequency = 0;
    int maxCount = -1;
    for (auto i = peaks.constBegin(); i < peaks.constEnd(); ++i) {
        int currentCount = 0;
        for (auto j = peaks.constBegin(); j < peaks.constEnd(); ++j) {
            if (i == j) continue;
            qreal ratio = *j / *i;
            int nearestInt = qRound(ratio);
            qreal remainder = qAbs(ratio - nearestInt);
            if (remainder < nearestInt * interval)
                currentCount++;
        }
        if (currentCount > maxCount) {
            candidateFrequency = *i;
            maxCount = currentCount;
        }
    }
    return candidateFrequency;
}

QList<qreal> Analyzer::interpolatePeaks(Spectrum spectrum, int numPeaks) const
{
    numPeaks = qMax(1, numPeaks);
    QList<qreal> peaks;
    peaks.reserve(numPeaks);

    // Compute central differences
    QList<qreal> derivative;
    derivative.reserve(spectrum.size());
    for (auto s = spectrum.constBegin() + 2; s < spectrum.constEnd() - 1; ++s) {
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
    for (auto s = spectrum.constBegin() + 1; s < spectrum.constEnd() - 1; ++s)
        averagePower += s->amplitude;
    averagePower /= spectrum.size() - 2;
    const qreal minPower = averagePower / 5;
    const qreal minSlope = -0.2;

    // Locate zero crossings
    int n = 0;
    QList<int> binPeaks;
    for (int i = 1; n < numPeaks && i < derivative.size() - 1; ++i) {
        if ( derivative.at(i) < minSlope && derivative.at(i - 1) > 0 && derivative.at(i + 1) < 0 &&
            spectrum.at(2 + i).frequency > 40 && spectrum.at(2 + i).amplitude > minPower)
        {
            binPeaks.append(2 + i);
            ++n;
        }
    }

    for (int i = 0; i < binPeaks.size(); ++i) {
        // Interpolate
        int k = binPeaks.at(i);
        qreal peak = k;
        if (k > 1 && k < spectrum.size() - 1) {
            qreal delta;
            switch(KTunerConfig::windowFunction()) {
            case KTunerConfig::NoWindow:
                // Interpolate using the complex coefficients
                delta = -std::real((m_output[k+1] - m_output[k-1]) /
                    (2.0 * m_output[k] - m_output[k+1] - m_output[k-1]));
                break;
            default:
                // This interpolation works better with window functions
                delta = std::real(0.6*(m_output[k-1] - m_output[k+1]) /
                    (m_output[k-1] + 2.0*m_output[k] + m_output[k+1]));
                break;
            }
            peak += delta;
        }
        peaks.append(peak * m_currentFormat.sampleRate() / m_sampleSize);
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

void Analyzer::averageSpectra()
{
    for (int i = 0; i < m_spectrum.size(); ++i) {
        qreal sum = 0;
        foreach (const Spectrum s, m_spectrumHistory) {
            sum += s.at(i).amplitude;
        }
        m_spectrum[i].amplitude = sum / m_numSpectra;
    }
}

#include "analyzer.moc"
