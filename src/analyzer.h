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

#ifndef ANALYZER_H
#define ANALYZER_H

#include "tone.h"

#include <QObject>
#include <QAudioBuffer>
#include <QVector>

// Include std complex first to allow complex arithmetic
#include <complex.h>
#include <fftw3.h>

class QAudioInput;
class QIODevice;

typedef QVector<Tone> Spectrum;

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

public:
    enum State {
        Loading,
        Ready,
        Processing,
        CalibratingFilter
    };

    explicit Analyzer(QObject* parent = 0);
    ~Analyzer();
    State state() const;
    static void spectrumSmooth(Spectrum &spectrum, quint32 times = 1);

    static const Spectrum NullResult;
    
signals:
    void done(const Spectrum harmonics, const Spectrum spectrum, const Spectrum autocorrelation);
    void stateChanged(State newState);
    
public slots:
    void doAnalysis(const QAudioBuffer &input);
    void setNoiseFilter(bool enable = true);
    void resetFilter();

private slots:
    void init();
    
private:
    void calculateWindow();
    Spectrum interpolatePeaks(int numPeaks = 1) const;
    template<typename T>
    void extractAndScale(const QAudioBuffer &input);
    void preProcess(const QAudioBuffer &input);
    void processFilter();
    void processSpectrum();
    Spectrum determineFundamental() const;
    void setState(State newState);
    QVector<int> findPeakIndices(const Spectrum &input) const;
    bool isPeak(const Tone *d) const;
    
    State m_state;  // Execution state
    bool m_calibrateFilter;  // Whether to calibrate a new noise filter
    quint32 m_sampleSize;  // Number of samples for spectral analysis
    quint32 m_outputSize;  // Number of elements in the output vector
    QAudioFormat m_currentFormat;
    Spectrum m_noiseSpectrum;
    quint32 m_numNoiseSegments; // Average over this many segments for the noise filter
    quint32 m_filterPass;
    
    // DFT variables
    QVector<double> m_window;
    QVector<double> m_input;
    QVector<std::complex<double>> m_output;
    fftw_plan m_plan;
    fftw_plan m_ifftPlan;
    Spectrum m_spectrum;
    
    // Spectral averaging
    quint32 m_numSpectra;
    quint32 m_currentSpectrum;
    QVector<Spectrum> m_spectrumHistory;
};

#endif // ANALYZER_H
