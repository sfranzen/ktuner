/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright 2016  Steven Franzen <email>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "analyzer.h"

#include <QAudioInput>
#include <QAudioDeviceInfo>
#include <QDataStream>
#include <QIODevice>
#include <QtMath>
#include <QDebug>

#include <limits.h>

Analyzer::Analyzer(QObject* parent)
    : QObject(parent)
    , m_sampleSize(4096)
    , m_hpsDepth(5)
    , m_windowFunction(DefaultWindowFunction)
    , m_numSpectra(3)
    , m_currentSpectrum(0)
{
    init();
    m_ready = true;
}

void Analyzer::init()
{    
    m_outputSize = 0.5 * m_sampleSize + 1;
    m_window.resize(m_sampleSize);
    calculateWindow();
    m_input.fill(0, m_sampleSize);
    m_output.fill(0, m_outputSize);
    m_spectrum.fill(0, m_outputSize);
    m_harmonicProductSpectrum.fill(0, (m_outputSize - 1) / m_hpsDepth);
    m_spectrumHistory.fill(m_spectrum, m_numSpectra);
    
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
    preProcess(input, format);

    // Obtain frequency information
    fftw_execute(m_plan);    
    for (uint i = 1; i < m_outputSize; ++i) {
        m_spectrum[i].frequency = qreal(i) * format.sampleRate() / m_sampleSize;
        m_spectrum[i].amplitude = qPow(std::abs(m_output.at(i)), 2) /  m_sampleSize;
    }
    
    // Harmonic Product spectrum
    for (int k = 0; k < m_harmonicProductSpectrum.size(); ++k) {
        m_harmonicProductSpectrum[k].frequency = m_spectrum.at(k).frequency;
        m_harmonicProductSpectrum[k].amplitude = m_spectrum.at(k).amplitude;
        for (uint n = 2; n <= m_hpsDepth; ++n) {
            m_harmonicProductSpectrum[k].amplitude *= m_spectrum.at(k*n).amplitude;
        }
        m_harmonicProductSpectrum[k].amplitude = qPow(m_harmonicProductSpectrum.at(k).amplitude, qreal(1) / m_hpsDepth);
    }
    
    m_spectrumHistory[m_currentSpectrum] = m_spectrum;
    m_currentSpectrum = (m_currentSpectrum + 1) % m_numSpectra;
    averageSpectra();
    
    const qreal estimate = interpolatePeakLocation(m_spectrum);
    emit done(estimate * format.sampleRate() / m_sampleSize, m_spectrum);
    m_ready = true;
}

qreal Analyzer::interpolatePeakLocation(Spectrum spectrum) const
{
    // Find the DFT bin with the largest amplitude
    int k = 1;
    qreal peakAmp = 0;
    for (int i = 1; i < spectrum.size(); ++i) {
        if (spectrum.at(i).amplitude > peakAmp &&
                spectrum.at(i).frequency >= 50)
        {
            k = i;
            peakAmp = spectrum.at(i).amplitude;
        }
    }
    
    if (k <= 1)
        return 1;
    else if (k >= spectrum.size() - 1)
        return spectrum.size();
    else {
        // Quadratic interpolation
        const Tone* y = spectrum.constData();
//         const qreal delta = 0.5 * (y[k-1].amplitude - y[k+1].amplitude) /
//                (y[k-1].amplitude - 2*y[k].amplitude + y[k+1].amplitude);
//         return (k + delta);

        // Interpolate using the complex coefficients
        const qreal delta = -std::real((m_output[k+1] - m_output[k-1]) /
                                       (2.0 * m_output[k] - m_output[k+1] - m_output[k-1]));
        return (k + delta);
    }
}

void Analyzer::calculateWindow()
{
    for (uint i = 0; i < m_sampleSize; ++i) {
        switch(m_windowFunction) {
        case NoWindow:
            m_window[i] = 1.0;
            break;
        case HannWindow:
            m_window[i] = 0.5 * (1 - qCos((2 * M_PI * i) / (m_sampleSize - 1)));
            break;
        case GaussianWindow:
            m_window[i] = qExp(-0.5 * qPow( (i - 0.5 * (m_sampleSize - 1)) /
                                            (0.4 * 0.5 * (m_sampleSize - 1)), 2));
            break;
        default:
            Q_UNREACHABLE();
        }
    }
}

void Analyzer::preProcess(QByteArray input, const QAudioFormat &format)
{
    // If not enough data is provided, pad with zeros
    const int difference = m_sampleSize * format.sampleSize() / 8 - input.size();
    if (difference > 0) {
        qDebug() << "Analyzer input too short, padding with zeros.";
        input.append(QByteArray(difference, 0));
    }
    
    // Extract and scale the audio samples from the buffer
    const char *ptr = input.constData();
    for (uint i = 0; i < m_sampleSize; ++i) {
//         qreal theta = (qreal)i * 2 * M_PI /  22050;
//         m_input[i] = 5.0 + 1.0 * qCos(82.0 * theta) + 0.5 * qCos(330.0 * theta);
        const qint16 sample = *reinterpret_cast<const qint16*>(ptr);
        m_input[i] = qreal(sample) / std::numeric_limits<qint16>::max();
        ptr += format.sampleSize() / 8;
    }

    // Find a simple least squares fit y = ax + b to the scaled input
    const qreal xMean = 0.5 * m_sampleSize;
    const qreal sum = std::accumulate(m_input.constBegin(), m_input.constEnd(), 0.0);
    const qreal yMean = sum / m_sampleSize;
    qreal covXY = 0, varX = 0; // Cross-covariance and variance
    for (uint x = 0; x < m_sampleSize; ++x) {
        covXY += (x - xMean) * (m_input.at(x) - yMean);
        varX += qPow(x - xMean, 2.0);
    }
    const qreal a = covXY / varX;
    const qreal b = yMean - a * xMean;

    // Subtract this fit and apply the window function
    for (uint x = 0; x < m_sampleSize; ++x) {
        m_input[x] = m_window.at(x) * (m_input.at(x) - (a * x + b));
    }
}

void Analyzer::averageSpectra()
{
    for (int i = 0; i < m_spectrum.size(); ++i) {
        m_spectrum[i].amplitude = 0;
        foreach (const Spectrum s, m_spectrumHistory) {
            m_spectrum[i].amplitude += s.at(i).amplitude;
        }
        m_spectrum[i].amplitude /=  m_numSpectra;
    }
}

void Analyzer::setSampleSize(uint n)
{
    m_sampleSize = qMax(n, 1u);
    init();
    emit sampleSizeChanged(m_sampleSize);
}

void Analyzer::setNumSpectra(uint num)
{
    num = qMax(num, 1u);
    if (m_numSpectra == num) {
        return;
    } else if (m_numSpectra < num) {
        for (uint i = m_numSpectra; i < num; ++i)
            m_spectrumHistory.append(m_spectrum);
    } else {
        for (uint i = num - 1; i > num; --i)
            m_spectrumHistory.removeLast();
    }
    m_numSpectra = num;
    m_currentSpectrum %= num;
    emit numSpectraChanged(m_numSpectra);
}

void Analyzer::setWindowFunction(WindowFunction w)
{
    m_windowFunction = w;
    calculateWindow();
}

#include "analyzer.moc"
