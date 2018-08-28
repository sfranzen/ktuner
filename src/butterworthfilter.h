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

#include <QVector>
#include <QDebug>

#include <complex>

/**
 * @todo write docs
 */
class ButterworthFilter
{
public:
    typedef std::complex<qreal> creal;
    typedef QVector<creal> CVector;
    enum FilterType {
        LowPass,
        HighPass,
        BandStop,
        BandPass
    };

    // Create a Butterworth filter of the given order using the provided cutoff
    // frequencies (in Hz, 1 for low/highpass filters or 2 for bandpass/stop)
    ButterworthFilter(QVector<qreal> cutoff, quint16 order = 2, qreal sampleRate = 1, FilterType type = LowPass);
    // Return the frequency response for a given vector of frequencies
    CVector response(const QVector<qreal> freq) const;
    CVector response(const Spectrum spectrum) const;
    // Evaluate filter response at s = i*w
    creal operator()(const creal s) const;

private:
    // Generate prototype lowpass filter of the given order
    void generatePrototype();
    // Transform the prototype to the requested type and frequency band
    void analogFilterTransform();
    // Transform a vector in place by applying an operator to each element
    template<typename T, class UnaryOperator> static void vectorTransform(QVector<T> &v, UnaryOperator op);
    // Perform the order-doubling transform for band filters
    void bandTransform(CVector &v) const;
    // Product of vector elements
    template<typename T> static T prod(const QVector<T> v);
    // Product of vector elements, pre-transformed by an operator
    template<typename T, class UnaryOperator> static T prod(const QVector<T> v, UnaryOperator op);

    QVector<qreal> m_cutoff;
    quint16 m_order;
    qreal m_sampleRate;
    FilterType m_type;
    qreal m_gain;
    CVector m_zeros;
    CVector m_poles;
};

QDebug& operator<<(QDebug &d, ButterworthFilter::creal c);

#endif // BUTTERWORTHFILTER_H
