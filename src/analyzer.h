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

#ifndef ANALYZER_H
#define ANALYZER_H

#include "tone.h"
#include "pitchtable.h"

#include <QObject>
#include <QAudioFormat>
#include <QByteArray>

// Include std complex first to allow complex arithmetic
#include <complex.h>
#include <fftw3.h>

class QAudioInput;
class QIODevice;

typedef QVector<Tone> Spectrum;

enum WindowFunction {
    NoWindow,
    HannWindow,
    GaussianWindow
};

const WindowFunction DefaultWindowFunction = NoWindow;

/* The Analyzer class determines the fundamental frequency in a series of audio
 * samples.
 * 
 * Analysis starts by preprocessing the raw audio input to scale it by the
 * maximum sample value, remove a linear least squares fit and apply a windowing
 * function. The resulting input array is transformed by FFTW's DFT algorithm
 * and its output used to calculate the power spectrum. This is followed by
 * calculation of the Harmonic Product Spectrum in order to find the fundamental 
 * frequency bin. Finally, the exact peak frequency is estimated by 
 * interpolation.
 */
class Analyzer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(WindowFunction windowFunction READ windowFunction WRITE setWindowFunction)
    Q_PROPERTY(uint sampleSize READ sampleSize WRITE setSampleSize NOTIFY sampleSizeChanged)
    
    void calculateWindow();
    qreal interpolatePeakLocation(Spectrum spectrum) const;
    void preProcess(QByteArray input, int bytesPerSample);
    
    bool m_ready;   // Execution state
    uint m_sampleSize;  // Number of samples for spectral analysis
    uint m_outputSize;  // Number of elements in the output vector
    int m_bufferLength;
    int m_bytesPerSample;
    uint m_hpsDepth;    // Number of harmonics included in HPS
    
    // DFT variables
    WindowFunction m_windowFunction;
    QVector<double> m_window;
    QVector<double> m_input;
    QVector<std::complex<double>> m_output;
    fftw_plan m_plan;
    Spectrum m_spectrum;
    Spectrum m_harmonicProductSpectrum;

public:
    Analyzer(QObject* parent = 0);
    ~Analyzer();
    bool isReady() const;
    uint outputSize() const { return m_outputSize; }
    WindowFunction windowFunction() const { return m_windowFunction; }
    void setWindowFunction(WindowFunction w);
    uint sampleSize() const { return m_sampleSize; }
    void setSampleSize(uint n);
    
signals:
    void done(const qreal frequency, const Spectrum spectrum);
    void sampleSizeChanged(const uint n);
    
public slots:    
    void doAnalysis(QByteArray input, const QAudioFormat *format);
};

#endif // ANALYZER_H
