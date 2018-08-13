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

#include "spectrum.h"

#include <math.h>

Spectrum::operator QVector<QPointF>() const
{
    QVector<QPointF> result(size());
    auto r = result.begin();
    for (const auto &t : *this) {
        *r = t;
        ++r;
    }
    return result;
}

QVector<Spectrum::const_iterator> Spectrum::findPeaks(qreal minimum) const
{
    QVector<Spectrum::const_iterator> peaks;
    peaks.reserve(size());

    // Compute central differences, smooth the result and locate the zero
    // crossings
    Spectrum derivative = computeDerivative();
    derivative.smooth();
    auto i = constBegin() + 1;
    const auto dEnd = derivative.constEnd() - 1;
    for (auto d = derivative.constBegin() + 1; d < dEnd; ++d, ++i) {
        if (i->amplitude > minimum && isNegativeZeroCrossing(d))
            peaks.append(i);
    }
    return peaks;
}

Spectrum Spectrum::computeDerivative() const
{
    static Spectrum derivative;
    derivative.resize(size());
    const auto iBegin = constBegin();
    const auto iEnd = constEnd();
    const qreal dx = (iBegin + 1)->frequency;

    // Use one-sided differences for the first and last values, and central
    // differences for all others
    derivative[0] = Tone(iBegin->frequency, ((iBegin + 1)->amplitude - iBegin->amplitude) / dx);
    derivative[size()-1] = Tone(iEnd->frequency, (iEnd->amplitude - (iEnd - 1)->amplitude) / dx);
    auto d = derivative.begin() + 1;
    for (auto i = iBegin + 1; i < iEnd - 1; ++d, ++i) {
        const qreal dy = (i + 1)->amplitude - (i - 1)->amplitude;
        *d = Tone(i->frequency, 0.5 * dy / dx);
    }
    return derivative;
}

void Spectrum::smooth()
{
    const auto spectrumEnd = end() - 1;
    for (auto s = begin() + 1; s < spectrumEnd; ++s)
        s->amplitude = ((s - 1)->amplitude + s->amplitude + (s + 1)->amplitude) / 3;
}

inline bool Spectrum::isNegativeZeroCrossing(Spectrum::const_iterator d) const
{
    return (d - 1)->amplitude > 0 && (d->amplitude < 0 || qFuzzyIsNull(d->amplitude));
}

inline Tone Spectrum::quadraticInterpolation(Spectrum::const_iterator peak) const
{
    const auto num = (peak-1)->amplitude - (peak+1)->amplitude;
    const auto delta = 0.5 * num / ((peak-1)->amplitude - 2 * peak->amplitude + (peak+1)->amplitude);
    const auto dx = peak->frequency - (peak-1)->frequency;
    return Tone(peak->frequency + delta * dx, peak->amplitude - 0.25 * num * delta);
}

// Is more accurate but returns NaN on negative input
inline Tone Spectrum::quadraticLogInterpolation(Spectrum::const_iterator peak) const
{
    const auto num = std::log10((peak-1)->amplitude / (peak+1)->amplitude);
    const auto delta = 0.5 * num / std::log10((peak-1)->amplitude * (peak+1)->amplitude / std::pow(peak->amplitude, 2));
    const auto dx = peak->frequency - (peak-1)->frequency;
    return Tone(peak->frequency + delta * dx, peak->amplitude - 0.25 * num * delta);
}
