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

#include "pitchtable.h"

#include <QStringList>
#include <math.h>

PitchTable::PitchTable(qreal concert_A4, Notation notation)
    : m_A4(concert_A4)
{
    QStringList pitchClasses;
    switch (notation) {
    case Notation::WesternSharps:
        pitchClasses = QStringList {"C", "C♯", "D", "D♯", "E", "F", "F♯", "G", "G♯", "A", "A♯", "B"};
        break;
    case Notation::WesternFlats:
        pitchClasses = QStringList {"C", "D♭", "D", "E♭", "E", "F", "G♭", "G", "A♭", "A", "B♭", "B"};
        break;
    }
    // Calculate pitches from C0 to E9, which is 112 semitones
    for (int i = 0; i < 112; ++i) {
        const auto frequency = C0() * std::pow(2.0, qreal(i) / 12);
        m_table.insert(frequency, Note(frequency, pitchClasses.at(i % 12), QString::number(i / 12)));
    }
}

qreal PitchTable::C0() const
{
    return m_A4 * std::pow(2.0, -4.75);
}

Note PitchTable::closestNote(qreal freq) const
{
    if (freq < m_table.firstKey())
        return m_table.first();
    else if (freq > m_table.lastKey())
        return m_table.last();

    auto lowerBound = m_table.lowerBound(freq);
    if (freq < 0.5 * ((lowerBound - 1).key() + lowerBound.key()))
        return (lowerBound - 1).value();
    else
        return lowerBound.value();
}
