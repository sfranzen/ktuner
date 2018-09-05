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

#include <QAudioBuffer>
#include <QDebug>

#include <math.h>
#include <functional>

#include <fftw3.h>

Analyzer::Analyzer(QObject* parent)
    : QObject(parent)
    , m_state(Loading)
    , m_binFreq(0)
    , m_numNoiseSegments(10)
    , m_filterPass(0)
    , m_plan(nullptr)
    , m_ifftPlan(nullptr)
    , m_currentSpectrum(0)
{
    init();
    connect(KTunerConfig::self(), &KTunerConfig::configChanged, this, &Analyzer::init);
    connect(KTunerConfig::self(), &KTunerConfig::noiseFilterChanged, this, &Analyzer::setNoiseFilter);
}

void Analyzer::init()
{
    setState(Loading);
    if (m_sampleSize != (quint32)KTunerConfig::segmentLength()) {
        m_sampleSize = KTunerConfig::segmentLength();
        m_outputSize = m_sampleSize + 1;
        m_window.resize(m_sampleSize);
        m_input.resize(2 * m_sampleSize);
        m_output.resize(m_outputSize);
        m_spectrum.resize(m_outputSize);
        m_noiseSpectrum.resize(m_outputSize);

        // FFTW and C++(99) complex types are binary compatible
        auto output = reinterpret_cast<fftw_complex*>(m_output.data());
        m_plan = fftw_plan_dft_r2c_1d(m_input.size(), m_input.data(), output, FFTW_MEASURE);
        m_ifftPlan = fftw_plan_dft_c2r_1d(m_input.size(), output, m_input.data(), FFTW_ESTIMATE);
    }
    if (m_numSpectra != (quint32)KTunerConfig::numSpectra()) {
        m_numSpectra = KTunerConfig::numSpectra();
        m_currentSpectrum %= m_numSpectra;
        m_spectrumHistory.fill(m_spectrum, m_numSpectra);
    }
    m_binFreq = qreal(KTunerConfig::sampleRate()) / m_input.size();
    calculateWindow();
    setNoiseFilter(KTunerConfig::enableNoiseFilter());
    setFftFilter();
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

    getSpectrum();
    if (m_calibrateFilter)
        calibrateFilter();
    processSpectrum();

    // Finally, compute the normalised ACF and frequency estimate
    getAcf();
    const auto snac = computeSnac(m_input, processedInput);
    const auto snacPeak = determineSnacFundamental(snac);
    Spectrum snacPeaks;
    if (snacPeak.frequency > 0)
        snacPeaks << snacPeak;

    // The accuracy of the obtained fundamental is fair, but can be improved
    // using the accurate power spectrum stored earlier, which also allows
    // identifying overtones
    const auto harmonics = findHarmonics(m_spectrum, m_currentFormat.sampleRate() / snacPeak.frequency);

    // Report analysis results
    setState(Ready);
    emit done(harmonics, m_spectrum, snac, snacPeaks);
}

void Analyzer::getSpectrum()
{
    fftw_execute(m_plan);
    // Extract the spectrum from the output. The zeroth output element is the
    // gain, which can be disregarded.
    auto o = m_output.constBegin() + 1;
    auto f = m_filter.constBegin() + 1;
    auto s = m_spectrum.begin() + 1;
    for (int i = 1; i < m_output.size(); ++i, ++o, ++s, ++f) {
        s->frequency = i * m_binFreq;
        s->amplitude = std::abs(*f * *o);
    }
}

void Analyzer::getAcf()
{
    // Prepare the output vector and compute the autocorrelation function,
    // which replaces m_input
    m_output[0] = 0;
    auto s = m_spectrum.constBegin() + 1;
    for (auto o = m_output.begin() + 1; o < m_output.end(); ++o, ++s)
        *o = std::pow(s->amplitude, 2);
    fftw_execute(m_ifftPlan);
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
    if (!enable) {
        m_filterPass = 0;
        m_noiseSpectrum.fill(0);
    }
}

void Analyzer::setFftFilter()
{
    m_filter.clear();
    m_filter.reserve(m_outputSize);
    auto filter = ButterworthFilter(75, 15000, 4, m_binFreq * m_input.size());
    for (int i = 0; i < m_output.size(); ++i)
        m_filter << filter(i * m_binFreq);
}

void Analyzer::resetFilter()
{
    m_calibrateFilter = true;
    m_filterPass = 0;
}

void Analyzer::calculateWindow()
{
    std::function<qreal(int)> wFunction = [](int){ return 1; };
    switch(KTunerConfig::windowFunction()) {
    default:
        break;
    case WindowFunction::Hann:
        wFunction = [&](int i){ return 0.5 * (1 - std::cos((2 * M_PI * i) / (m_sampleSize - 1))); };
        break;
    case WindowFunction::Gaussian:
        wFunction = [&](int i){
            return std::exp(-0.5 * std::pow((i - 0.5 * (m_sampleSize - 1)) / (0.25 * 0.5 * (m_sampleSize - 1)), 2));
        };
        break;
    }
    for (quint32 i = 0; i < m_sampleSize; ++i)
        m_window[i] = wFunction(i);
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
    const qreal scale = std::pow(2, 8*sizeof(T) - 1);
    const uint end = std::min(m_sampleSize, (uint)input.sampleCount());
    const auto iBegin = m_input.begin();
    for (auto i = iBegin; i < iBegin + end; ++i, ++data)
        *i = *data / scale;
}

void Analyzer::calibrateFilter()
{
    if (m_filterPass == 0) {
        ++m_filterPass;
        m_noiseSpectrum = m_spectrum;
        for (auto &t : m_noiseSpectrum)
            t.amplitude /= m_numNoiseSegments;
    } else if (m_filterPass < m_numNoiseSegments) {
        ++m_filterPass;
        auto s = m_spectrum.constBegin();
        for (auto &t : m_noiseSpectrum)
            t.amplitude += s++->amplitude / m_numNoiseSegments;
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
        s->amplitude = std::max(0.0,  *a / m_numSpectra - n->amplitude);
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
    const auto peaks = snac.findPeaks();
    if (peaks.isEmpty())
        return result;

    // First find the highest peak other than the first SNAC value, which
    // should be 1.0, then pick the first peak after the first zero crossing
    // that exceeds 0.8 times that value
    const auto maxPeak = *std::max_element(peaks.constBegin(), peaks.constEnd(), [](const Tone *t1, const Tone *t2) {
        return *t1 < *t2;
    });
    const auto zeros = snac.findZeros(1);
    auto pick = std::find_if(peaks.begin(), peaks.end(), [&](const Tone *t) {
        return t > zeros.first() && t->amplitude > 0.8 * maxPeak->amplitude;
    });
    if (pick != peaks.end()) {
        result = quadraticInterpolation(*pick);
        Q_ASSERT(result.frequency > 0);
    }
    return result;
}

// Algorithm: first interpolate the spectral peak corresponding to fApprox,
// then locate the (near-)integer multiples of its frequency
Spectrum Analyzer::findHarmonics(const Spectrum spectrum, qreal fApprox) const
{
    Spectrum harmonics;
    if (fApprox <= 0 || std::isinf(fApprox))
        return harmonics;
    const auto peaks = spectrum.findPeaks(0.01);
    if (peaks.isEmpty())
        return harmonics;

    harmonics.reserve(peaks.size());
    const auto iFund = std::floor(fApprox / m_binFreq) + 1;
    const auto fundamental = quadraticInterpolation(&spectrum[iFund]);
    harmonics.append(fundamental);
    for (const auto peak : peaks) {
        if (peak->frequency > fundamental.frequency) {
            const Tone t = quadraticInterpolation(peak);
            const qreal ratio = t.frequency / fundamental.frequency;
            if (qAbs(1200 * std::log2(ratio / qRound(ratio))) < 10)
                harmonics.append(t);
        }
    }
    return harmonics;
}
