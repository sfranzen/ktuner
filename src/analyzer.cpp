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

#include <math.h>

const Spectrum Analyzer::NullResult = Spectrum() << Tone(0, 0);

Analyzer::Analyzer(QObject* parent)
    : QObject(parent)
    , m_state(Loading)
    , m_numNoiseSegments(10)
    , m_filterPass(0)
    , m_plan(Q_NULLPTR)
    , m_ifftPlan(Q_NULLPTR)
    , m_currentSpectrum(0)
{
    init();
    connect(KTunerConfig::self(), &KTunerConfig::configChanged, this, &Analyzer::init);
    connect(KTunerConfig::self(), &KTunerConfig::noiseFilterChanged, this, &Analyzer::setNoiseFilter);
}

void Analyzer::init()
{
    setState(Loading);
    m_sampleSize = KTunerConfig::segmentLength();
    m_numSpectra = KTunerConfig::numSpectra();
    m_currentSpectrum %= m_numSpectra;

    m_window.resize(m_sampleSize);
    calculateWindow();
    m_input.fill(0, 2 * m_sampleSize);
    m_outputSize = m_input.size() / 2 + 1;
    m_output.fill(0, m_outputSize);
    m_spectrum.fill(0, m_outputSize);
    m_noiseSpectrum.fill(0, m_outputSize);
    m_spectrumHistory.fill(m_spectrum, m_numSpectra);
    setNoiseFilter(KTunerConfig::enableNoiseFilter());

    // FFTW and C++(99) complex types are binary compatible
    auto output = reinterpret_cast<fftw_complex*>(m_output.data());
    m_plan = fftw_plan_dft_r2c_1d(m_input.size(), m_input.data(), output, FFTW_MEASURE);
    m_ifftPlan = fftw_plan_dft_c2r_1d(m_input.size(), output, m_input.data(), FFTW_ESTIMATE);
    setState(Ready);
}

Analyzer::~Analyzer()
{
    fftw_destroy_plan(m_plan);
    fftw_destroy_plan(m_ifftPlan);
    fftw_cleanup();
}

void Analyzer::doAnalysis(const QAudioBuffer &input)
{
    if (m_state != Ready)
        return;
    if (m_calibrateFilter)
        setState(CalibratingFilter);
    else
        setState(Processing);

    // Process the bytearray into m_input and store a copy for computation of
    // the SNAC function
    m_currentFormat = input.format();
    preProcess(input);
    auto processedInput = m_input;
    processedInput.detach();

    // Extract the spectrum from the output. The zeroth output element is the
    // gain, which can be disregarded.
    fftw_execute(m_plan);
    for (int i = 1; i < m_output.size(); ++i) {
        m_spectrum[i].frequency = qreal(i) * input.format().sampleRate() / m_input.size();
        m_spectrum[i].amplitude = std::abs(m_output.at(i));
    }
    if (m_calibrateFilter)
        processFilter();
    processSpectrum();

    // Prepare the output vector and compute the autocorrelation function,
    // which replaces m_input
    m_output[0] = 0;
    auto s = m_spectrum.constBegin() + 1;
    for (auto o = m_output.begin() + 1; o < m_output.end(); ++o, ++s)
        *o = qPow(s->amplitude, 2);
    fftw_execute(m_ifftPlan);

    // Finally, compute the normalised ACF and frequency estimate
    const auto snac = computeSnac(m_input, processedInput);
    const auto fundamental = determineSnacFundamental(snac);

    // The accuracy of the obtained fundamental is fair, but can be improved
    // using the accurate power spectrum stored earlier, which also allows
    // identifying overtones
    const auto harmonics = findHarmonics(m_spectrum, fundamental);

    // Report analysis results
    emit done(harmonics, m_spectrum, snac);
    setState(Ready);
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

void Analyzer::setNoiseFilter(bool enable)
{
    m_calibrateFilter = enable;
    if (!enable)
        m_filterPass = 0;
}

void Analyzer::resetFilter()
{
    m_calibrateFilter = true;
    m_filterPass = 0;
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
                (0.25 * 0.5 * (m_sampleSize - 1)), 2));
                break;
            default:
                Q_UNREACHABLE();
        }
    }
}

void Analyzer::preProcess(const QAudioBuffer &input)
{
    m_input.fill(0);
    switch (input.format().sampleSize()) {
    case 8:
        extractAndScale<qint8>(input);
        break;
    case 16:
        extractAndScale<qint16>(input);
        break;
    case 32:
        extractAndScale<qint32>(input);
        break;
    case 64:
        extractAndScale<qint64>(input);
        break;
    }

    // Find a simple least squares fit y = ax + b to the scaled input
    const qreal xMean = 0.5 * (m_sampleSize + 1);
    const qreal sum = std::accumulate(m_input.constBegin(), m_input.constBegin() + m_sampleSize - 1, 0.0);
    const qreal yMean = sum / m_sampleSize;
    qreal covXY = 0, varX = 0; // Cross-covariance and variance

    auto y = m_input.constBegin();
    const auto yEnd = m_input.constEnd();
    for (quint32 x = 0; y < yEnd; ++x, ++y) {
        const auto dx = x - xMean;
        covXY += dx * (*y - yMean);
        varX += dx * dx;
    }
    const qreal a = covXY / varX;
    const qreal b = yMean - a * xMean;

    // Subtract this fit and apply the window function
    auto i = m_input.begin();
    auto w = m_window.constBegin();
    for (quint32 x = 0; w < m_window.constEnd(); ++i, ++w, ++x) {
        *i = *w * (*i - (a * x + b));
    }
}

template<typename T>
void Analyzer::extractAndScale(const QAudioBuffer &input)
{
    const T *data = input.constData<T>();
    const qreal scale = qPow(2, 8*sizeof(T) - 1);
    const uint end = qMin(m_sampleSize, (uint)input.sampleCount());
    const auto iBegin = m_input.begin();
    for (auto i = iBegin; i < iBegin + end; ++i, ++data)
        *i = *data / scale;
}

void Analyzer::processFilter()
{
    Spectrum::iterator i;
    Spectrum::const_iterator j;
    if (m_filterPass == 0) {
        m_noiseSpectrum.fill(0);
        for (i = m_noiseSpectrum.begin(), j = m_spectrum.constBegin(); i < m_noiseSpectrum.end(); ++i, ++j)
            i->frequency = j->frequency;
    }
    if (m_filterPass < m_numNoiseSegments) {
        m_filterPass++;
        for (i = m_noiseSpectrum.begin(), j = m_spectrum.constBegin(); i < m_noiseSpectrum.end(); ++i, ++j)
            i->amplitude += j->amplitude / m_numNoiseSegments;
    } else {
        m_filterPass = 0;
        m_calibrateFilter = false;
    }
}

void Analyzer::processSpectrum()
{
    m_spectrumHistory[m_currentSpectrum].swap(m_spectrum);
    m_currentSpectrum = (m_currentSpectrum + 1) % m_numSpectra;

    static QVector<qreal> average;
    average.fill(0, m_outputSize);

    const auto averageBegin = average.begin();
    const auto averageEnd = average.end();
    for (auto h = m_spectrumHistory.constBegin(); h < m_spectrumHistory.constEnd(); ++h) {
        auto hPoint = h->constBegin();
        for (auto a = averageBegin; a < averageEnd; ++a,  ++hPoint)
            *a +=  hPoint->amplitude;
    }

    const auto spectrumEnd = m_spectrum.end();
    auto n = m_noiseSpectrum.constBegin();
    auto a = average.constBegin();
    for (auto s = m_spectrum.begin(); s < spectrumEnd; ++s, ++n, ++a)
        s->amplitude = qMax(0.0,  *a / m_numSpectra - n->amplitude);
}

Spectrum Analyzer::computeSnac(const QVector<double> acf, const QVector<double> signal) const
{
    Spectrum snac(m_sampleSize);
    const quint32 W = m_sampleSize;
    qreal mSum = 2 * acf[0];
    int tau = 0;
    const auto snacEnd = snac.end();
    for (auto s = snac.begin(); s < snacEnd; ++s) {
        *s = Tone(tau, 2 * acf[tau] / mSum);
        tau++;
        const auto m1 = signal[tau - 1];
        const auto m2 = signal[W - tau];
        mSum -= m1 * m1 + m2 * m2;
    }
    return snac;
}

Tone Analyzer::determineSnacFundamental(const Spectrum snac) const
{
    Tone result;
    const auto peaks = findPeaks(snac);
    if (peaks.isEmpty())
        return result;

    // First find the highest peak other than the first SNAC value, which
    // should be 1.0, then pick the *first* peak exceeding 0.8 times that value
    const auto maxPeak = *std::max_element(peaks.constBegin() + 1, peaks.constEnd(), &Tone::compareAmplitude);
    for (const auto peak : peaks) {
        if (peak->amplitude > 0.8 * maxPeak->amplitude) {
            result = quadraticInterpolation(peak);
            result.frequency = m_currentFormat.sampleRate() / result.frequency;
            break;
        }
    }
    return result;
}

// Algorithm: first interpolate the spectral peak corresponding to fApprox,
// then locate the (near-)integer multiples of its frequency
Spectrum Analyzer::findHarmonics(const Spectrum spectrum, const Tone &fApprox) const
{
    Spectrum harmonics;
    const auto peaks = findPeaks(spectrum, 0.01);
    if (peaks.isEmpty())
        return NullResult;

    harmonics.reserve(peaks.size());
    const auto baseFreq = qreal(m_currentFormat.sampleRate()) / m_input.size();
    const auto iFund = qFloor(fApprox.frequency / baseFreq) + 1;
    const auto fundamental = quadraticLogInterpolation(&spectrum[iFund]);
    harmonics.append(fundamental);
    for (const auto peak : peaks) {
        if (peak->frequency > fundamental.frequency) {
            const Tone t = quadraticLogInterpolation(peak);
            const qreal ratio = t.frequency / fundamental.frequency;
            if (qAbs(1200 * std::log2(ratio / qRound(ratio))) < 10)
                harmonics.append(t);
        }
    }
    if (!harmonics.isEmpty())
        return harmonics;
    else
        return NullResult;
}

QVector<Spectrum::const_iterator> Analyzer::findPeaks(const Spectrum &input, qreal minimum)
{
    QVector<Spectrum::const_iterator> peaks;
    peaks.reserve(input.size());

    // Compute central differences
    static Spectrum derivative;
    derivative.reserve(input.size());
    derivative.clear();
    auto iBegin = input.constBegin();
    auto iEnd = input.constEnd();
    const qreal dx = (iBegin + 1)->frequency;
    derivative.append(Tone(iBegin->frequency, ((iBegin + 1)->amplitude - iBegin->amplitude) / dx));
    for (auto i = iBegin + 1; i < iEnd - 1; ++i) {
        qreal dy = (i + 1)->amplitude - (i - 1)->amplitude;
        derivative.append(Tone(i->frequency, 0.5 * dy / dx));
    }
    derivative.append(Tone(iEnd->frequency, (iEnd->amplitude - (iEnd - 1)->amplitude) / dx));

    // Smooth and locate peaks
    smooth(derivative);
    auto i = input.constBegin() + 1;
    for (auto d = derivative.constBegin() + 1; d < derivative.constEnd() - 1; ++d, ++i) {
        if (i->amplitude > minimum && isNegativeZeroCrossing(d))
            peaks.append(i);
    }
    return peaks;
}

void Analyzer::smooth(Spectrum &spectrum)
{
    const auto spectrumEnd = spectrum.end() - 1;
    for (auto s = spectrum.begin() + 1; s < spectrumEnd; ++s)
        s->amplitude = ((s - 1)->amplitude + s->amplitude + (s + 1)->amplitude) / 3;
}

inline bool Analyzer::isNegativeZeroCrossing(Spectrum::const_iterator d) {
    return (d - 1)->amplitude > 0 && (d->amplitude < 0 || qFuzzyIsNull(d->amplitude));
}

inline Tone Analyzer::quadraticInterpolation(Spectrum::const_iterator peak)
{
    const auto num = (peak-1)->amplitude - (peak+1)->amplitude;
    const auto delta = 0.5 * num / ((peak-1)->amplitude - 2 * peak->amplitude + (peak+1)->amplitude);
    const auto dx = peak->frequency - (peak-1)->frequency;
    return Tone(peak->frequency + delta * dx, peak->amplitude - 0.25 * num * delta);
}

// Is more accurate but returns NaN on negative input
inline Tone Analyzer::quadraticLogInterpolation(Spectrum::const_iterator peak)
{
    const auto num = std::log10((peak-1)->amplitude / (peak+1)->amplitude);
    const auto delta = 0.5 * num / std::log10((peak-1)->amplitude * (peak+1)->amplitude / qPow(peak->amplitude, 2));
    const auto dx = peak->frequency - (peak-1)->frequency;
    return Tone(peak->frequency + delta * dx, peak->amplitude - 0.25 * num * delta);
}
