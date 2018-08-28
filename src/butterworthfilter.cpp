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

ButterworthFilter::ButterworthFilter(QVector<qreal> cutoff, quint16 order, qreal sampleRate, FilterType type)
    : m_cutoff(cutoff)
    , m_order(order)
    , m_sampleRate(sampleRate)
    , m_type(type)
    , m_gain(1)
    , m_poles(order)
{
    vectorTransform(m_cutoff, [&](qreal f){ return f / sampleRate; });
    generatePrototype();
    analogFilterTransform();
}

ButterworthFilter::creal ButterworthFilter::operator()(const creal s) const
{
    const auto factor = [&](creal z) { return s - z; };
    const creal H = m_gain * prod(m_zeros, factor) / prod(m_poles, factor);
    return H;
}

ButterworthFilter::CVector ButterworthFilter::operator()(const QVector<qreal> freq) const
{
    CVector response(freq.size());
    const auto factor =  I / m_sampleRate;
    auto h = response.begin();
    for (const auto &f : freq) {
        *h++ = operator()(factor * f);
    }
    return response;
}

ButterworthFilter::CVector ButterworthFilter::operator()(const Spectrum spectrum) const
{
    CVector response(spectrum.size());
    const auto factor =  I / m_sampleRate;
    auto h = response.begin();
    for (const auto &s : spectrum) {
        *h++ = operator()(factor * s.frequency);
    }
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
    if (m_order == 0 || m_cutoff.isEmpty() || m_cutoff.size() > 2)
        return;

    // Prototype lowpass filter, which has a crossover of 1 rad/s
    auto p = m_poles.begin();
    for (int k = 1; k <= m_order; ++k, ++p)
        *p = std::exp(I * (2.0*k + m_order - 1) * M_PI / (2.0 * m_order));

    // Make middle pole purely real if order is an odd number
    if (m_order % 2 == 1)
        m_poles[(m_order - 1) / 2] = -1;
}

void ButterworthFilter::analogFilterTransform()
{
    const auto m = m_zeros.size();
    const auto n = m_poles.size();
    if (m > n || n == 0)
        return;

    // Define a number of constants to simplify expressions
    const auto w1 = m_cutoff.first();
    const auto w2 = m_cutoff.size() == 2 ? m_cutoff.last() : 2 * w1;
    const auto w0 = std::sqrt(w1 * w2);
    const auto wc = w2 - w1;

    std::function<creal(creal)> transform;
    if (m_type == LowPass || m_type == BandPass) {
        m_gain *= std::pow(wc, n - m);
        if (m_type == LowPass) {
            transform = [&](creal z){ return z * w1; };
            vectorTransform(m_zeros, transform);
            vectorTransform(m_poles, transform);
        } else { // BandPass
            bandTransform(m_poles);
            if (m_zeros.isEmpty())
                m_zeros.fill(0, n);
            else {
                bandTransform(m_zeros);
                if (n > m)
                    m_zeros << CVector(0, n - m);;
            }
        }
    } else {
        m_gain *= std::real(prod(m_zeros, std::negate<creal>()) / prod(m_poles, std::negate<creal>()));
        if (m_type == HighPass) {
            transform = [&](creal z){ return w1 / z; };
            vectorTransform(m_poles, transform);
            if (m_zeros.isEmpty())
                m_zeros.fill(0, n);
            else {
                vectorTransform(m_zeros, transform);
                if (n > m)
                    m_zeros << CVector(0, n - m);
            }
        } else { // BandStop
            bandTransform(m_poles);
            const CVector zeros {I*w0, -I*w0};
            if (m_zeros.isEmpty()) {
                m_zeros.reserve(2*n);
                for (int i = 1; i <= 2*n; ++i)
                    m_zeros << zeros.at(i % 2);
            } else {
                bandTransform(m_zeros);
                if (n > m) {
                    m_zeros.reserve(2*n + n - m);
                    for (int i = 1; i <= n - m; ++i)
                        m_zeros << zeros.at(i % 2);
                }
            }
        }
    }
}

template<typename T, class UnaryOperator>
void ButterworthFilter::vectorTransform(QVector<T> &v, UnaryOperator op)
{
    std::transform(v.begin(), v.end(), v.begin(), op);
}

void ButterworthFilter::bandTransform(CVector &v) const
{
    const auto w1 = m_cutoff.first();
    const auto w2 = m_cutoff.size() == 2 ? m_cutoff.last() : 2 * w1;
    const auto wc = w2 - w1;
    const auto q = std::sqrt(w1 * w2) / wc;
    const auto n = v.size();
    v.resize(2 * n);
    for (auto e = v.begin(); e < v.begin() + n; ++e) {
        const auto offset = std::sqrt(*e**e - 4 * q*q);
        *(e + n) = 0.5 * wc * (*e - offset);
        *e = 0.5 * wc * (*e + offset);
    }
}

template<typename T> inline T ButterworthFilter::prod(const QVector<T> v)
{
    return std::accumulate(v.constBegin(), v.constEnd(), T(1.0), std::multiplies<T>());
}

template<typename T, class UnaryOperator> inline T ButterworthFilter::prod(const QVector<T> v, UnaryOperator op)
{
    QVector<T> w(v.size());
    std::transform(v.begin(), v.end(), w.begin(), op);
    return prod(w);
}

QDebug& operator<<(QDebug &d, ButterworthFilter::creal c)
{
    const auto op = c.imag() < 0 ? " - " : " + ";
    d << c.real() << op << std::abs(c.imag()) << "i";
    return d;
}
