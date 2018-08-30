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

#include "butterworthfilter.h"

#include <functional>

const auto I = std::complex<qreal>(0, 1);

namespace {
    // Product of vector elements
    template<typename T> inline T prod(const QVector<T> v)
    {
        return std::accumulate(v.constBegin(), v.constEnd(), T(1.0), std::multiplies<T>());
    }

    // Product of vector elements, pre-transformed by an operator
    template<typename T, class UnaryOperator> inline T prod(const QVector<T> v, UnaryOperator op)
    {
        QVector<T> w(v.size());
        std::transform(v.begin(), v.end(), w.begin(), op);
        return prod(w);
    }

    // Transform a vector in place by applying an operator to each element
    template<typename T, class UnaryOperator> void vectorTransform(QVector<T> &v, UnaryOperator op)
    {
        std::transform(v.begin(), v.end(), v.begin(), op);
    }
}

ButterworthFilter::ButterworthFilter(qreal cutoffLow, qreal cutoffHigh, quint16 order, qreal sampleRate, FilterType type)
    : m_cutoff {cutoffLow, cutoffHigh}
    , m_poles(order)
    , m_gain(1)
    , m_sampleRate(sampleRate)
    , m_type(type)
    , m_order(order)
{
    vectorTransform(m_cutoff, [&](qreal f){ return f * 2 * M_PI / sampleRate; });
    generatePrototype();
    transformGain();
    transformPolesZeros();
}

ButterworthFilter::ButterworthFilter(qreal cutoffLow, qreal cutoffHigh, quint16 order, qreal sampleRate, bool pass)
    : ButterworthFilter(cutoffLow, cutoffHigh, order, sampleRate, pass ? BandPass : BandStop)
{
}

// Setting a virtual second cutoff twice as large simplifies algorithms
ButterworthFilter::ButterworthFilter(qreal cutoff, quint16 order, qreal sampleRate, bool lowPass)
    : ButterworthFilter(cutoff, 2 * cutoff, order, sampleRate, lowPass ? LowPass : HighPass)
{
}

ButterworthFilter::creal ButterworthFilter::operator()(const creal s) const
{
    const auto factor = [&](creal z) { return s - z; };
    const creal H = m_gain * prod(m_zeros, factor) / prod(m_poles, factor);
    return H;
}

ButterworthFilter::creal ButterworthFilter::operator()(const qreal f) const
{
    return operator()(2 * M_PI * I * f / m_sampleRate);
}

ButterworthFilter::CVector ButterworthFilter::operator()(const QVector<qreal> freq) const
{
    CVector response(freq.size());
    std::transform(freq.cbegin(), freq.cend(), response.begin(), [&](qreal f){ return operator()(f); });
    return response;
}

ButterworthFilter::CVector ButterworthFilter::operator()(const Spectrum spectrum) const
{
    CVector response(spectrum.size());
    std::transform(spectrum.cbegin(), spectrum.cend(), response.begin(), [&](Tone s){ return operator()(s.frequency); });
    return response;
}

void ButterworthFilter::operator+=(const ButterworthFilter& other)
{
    // Only these properties matter for evaluation
    m_gain *= other.m_gain;
    m_zeros << other.m_zeros;
    m_poles << other.m_poles;
}

ButterworthFilter operator+(ButterworthFilter f1, const ButterworthFilter f2)
{
    f1 += f2;
    return f1;
}

void ButterworthFilter::generatePrototype()
{
    // Prototype lowpass filter, which has a crossover of 1 rad/s
    auto p = m_poles.begin();
    for (int k = 1; k <= m_order; ++k, ++p)
        *p = std::exp(I * (2.0*k + m_order - 1) * M_PI / (2.0 * m_order));

    // Make middle pole purely real if order is an odd number
    if (m_order % 2 == 1)
        m_poles[(m_order - 1) / 2] = -1;
}

void ButterworthFilter::transformGain()
{
    if (m_type == LowPass || m_type == BandPass) {
        // wc reduces to w1 for the lowpass filter
        const auto wc = m_cutoff.last() - m_cutoff.first();
        m_gain *= std::pow(wc, m_poles.size() - m_zeros.size());
    } else
        m_gain *= std::real(prod(m_zeros, std::negate<creal>()) / prod(m_poles, std::negate<creal>()));
}

template<> void ButterworthFilter::transformPolesZeros<ButterworthFilter::LowPass>()
{
    auto transform = [&](creal z){ return m_cutoff.first() * z; };
    vectorTransform(m_zeros, transform);
    vectorTransform(m_poles, transform);
}

template<> void ButterworthFilter::transformPolesZeros<ButterworthFilter::HighPass>()
{
    auto transform = [&](creal z){ return m_cutoff.first() / z; };
    if (m_zeros.isEmpty())
        m_zeros.fill(0, m_poles.size());
    else {
        vectorTransform(m_zeros, transform);
        zeroPad();
    }
    vectorTransform(m_poles, transform);
}

template<> void ButterworthFilter::transformPolesZeros<ButterworthFilter::BandPass>()
{
    if (m_zeros.isEmpty())
        m_zeros.fill(0, m_poles.size());
    else {
        bandTransform(m_zeros);
        zeroPad();
    }
    bandTransform(m_poles);
}

template<> void ButterworthFilter::transformPolesZeros<ButterworthFilter::BandStop>()
{
    bandTransform(m_poles);
    const auto w0 = std::sqrt(prod(m_cutoff));
    const CVector zeros {I*w0, -I*w0};
    const auto n = m_poles.size();
    if (m_zeros.isEmpty()) {
        m_zeros.reserve(2 * n);
        for (int i = 1; i <= 2 * n; ++i)
            m_zeros << zeros.at(i % 2);
    } else {
        bandTransform(m_zeros);
        for (int i = 1; i < n - m_zeros.size(); ++i)
            m_zeros << zeros.at(i % 2);
    }
}

void ButterworthFilter::transformPolesZeros()
{
    switch(m_type) {
    case LowPass:
        transformPolesZeros<LowPass>();
        break;
    case HighPass:
        transformPolesZeros<HighPass>();
        break;
    case BandPass:
        transformPolesZeros<BandPass>();
        break;
    case BandStop:
        transformPolesZeros<BandStop>();
        break;
    }
}

void ButterworthFilter::zeroPad()
{
    const auto diff = m_poles.size() - m_zeros.size();
    if (diff > 0)
        m_zeros << CVector(0, diff);
}

void ButterworthFilter::bandTransform(CVector &v) const
{
    const auto wc = m_cutoff.last() - m_cutoff.first();
    const auto q2 = prod(m_cutoff) / wc / wc;
    const auto n = v.size();
    v.resize(2 * n);
    for (auto e = v.begin(); e < v.begin() + n; ++e) {
        const auto offset = std::sqrt(*e**e - 4 * q2);
        *(e + n) = 0.5 * wc * (*e - offset);
        *e = 0.5 * wc * (*e + offset);
    }
}

QDebug operator<<(QDebug d, ButterworthFilter::creal c)
{
    const auto op = c.imag() < 0 ? " - " : " + ";
    d.nospace() << c.real() << op << std::abs(c.imag()) << "i";
    return d.maybeSpace();
}
