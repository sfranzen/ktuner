/*
 * Copyright 2018 Steven Franzen <sfranzen85@gmail.com>
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

#ifndef BUTTERWORTHFILTER_H
#define BUTTERWORTHFILTER_H

#include "spectrum.h"

#include <QtGlobal>
#include <QVector>
#include <QDebug>

#include <complex>

/**
 * @todo write docs
 */
class ButterworthFilter
{
public:
    using creal = std::complex<qreal>;
    using CVector = QVector<creal>;

    // Create a Butterworth filter of the given order using the provided cutoff
    // frequencies (in Hz, 1 for low/highpass filters or 2 for bandpass/stop)
    ButterworthFilter(qreal cutoffLow, qreal cutoffHigh, quint16 order = 2, qreal sampleRate = 1, bool pass = true);
    ButterworthFilter(qreal cutoff, quint16 order = 2, qreal sampleRate = 1, bool lowPass = true);
    // Evaluate filter response at s = i*w
    creal operator()(creal s) const;
    // Evaluate at s = 2*pi * i * f
    creal operator()(qreal f) const;
    // Return the frequency response for a given vector of frequencies
    CVector operator()(const QVector<qreal> freq) const;
    CVector operator()(const Spectrum spectrum) const;
    // Add another filter to this one
    void operator+=(const ButterworthFilter &other);
    friend ButterworthFilter operator+(ButterworthFilter f1, const ButterworthFilter f2);

private:
    enum FilterType {
        LowPass,
        HighPass,
        BandStop,
        BandPass
    };

    ButterworthFilter(qreal cutoffLow, qreal cutoffHigh, quint16 order, qreal sampleRate, FilterType type);

    // Generate prototype lowpass filter of the given order
    void generatePrototype();
    // Transform the gain factor
    void transformGain();
    // Transform the prototype to the requested type and frequency band
    void transformPolesZeros();
    template<FilterType T> void transformPolesZeros();
    // Pad m_zeros to the same length as m_poles
    void zeroPad();
    // Perform the order-doubling transform for band filters
    void bandTransform(CVector &v) const;

    QVector<qreal> m_cutoff;
    CVector m_zeros;
    CVector m_poles;
    qreal m_gain;
    const qreal m_sampleRate;
    const FilterType m_type;
    const quint16 m_order;
};

QDebug operator<<(QDebug d, ButterworthFilter::creal c);

#endif // BUTTERWORTHFILTER_H
