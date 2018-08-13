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

#ifndef SPECTRUM_H
#define SPECTRUM_H

#include "tone.h"

#include <QVector>

/**
 * @todo write docs
 */
class Spectrum : public QVector<Tone>
{
public:
    using QVector::QVector;

    operator QVector<QPointF>() const;
    QVector<const_iterator> findPeaks(qreal minimum = 0) const;
    Spectrum computeDerivative() const;
    void smooth();
    bool isNegativeZeroCrossing(const_iterator d) const;
    Tone quadraticInterpolation(const_iterator peak) const;
    Tone quadraticLogInterpolation(const_iterator peak) const;
};

#endif // SPECTRUM_H
