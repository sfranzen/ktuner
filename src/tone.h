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

#ifndef TONE_H
#define TONE_H

#include <QtGlobal>
#include <QPointF>

struct Tone
{
    qreal frequency = 0;
    qreal amplitude = 0;

    Tone(qreal freq = 0, qreal amp = 0) : frequency(freq), amplitude(amp) {};

    operator QPointF() const { return {frequency, amplitude}; }
};

inline bool operator==(Tone t1, Tone t2)
{
    return t1.frequency == t2.frequency && t1.amplitude == t2.amplitude;
}

inline bool operator<(Tone t1, Tone t2) {
    return t1.amplitude < t2.amplitude;
}

#endif // TONE_H
