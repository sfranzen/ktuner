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

#include "pitchtable.h"

#include <QtMath>

PitchTable::PitchTable(qreal concert_A4, PitchNotation notation)
    : m_notation(notation)
    , m_concert_A4(concert_A4)
    , m_C0(concert_A4 * qPow(2.0, -4.75))
{
    switch (m_notation) {
        case PitchNotation::Western:
            m_pitchClasses  << "C" << "C♯" << "D" << "D♯" << "E" << "F"
                            << "F♯" << "G" << "G♯" << "A" << "A♯" << "B";
            break;
        default:
            Q_UNREACHABLE();
    }
    // Calculate pitches from C0 to E9, which is 112 semitones
    for (int i = 0; i < 112; ++i) {
        qreal frequency = m_C0 *  qPow(2.0, qreal(i) / 12);
        m_table.insert(frequency, new Note(frequency, m_pitchClasses.at(i % 12), i / 12));
    }
}

PitchTable::~PitchTable()
{
    qDeleteAll(m_table);
    m_table.clear();
}

qreal PitchTable::C0() const
{
    return m_C0;
}

Note* PitchTable::closestNote(const qreal freq) const
{
    if (freq < m_table.firstKey())
        return m_table.first();
    else if (freq > m_table.lastKey())
        return m_table.last();

    QMap<qreal, Note*>::const_iterator lowerBound = m_table.lowerBound(freq);
    if (freq < 0.5 * ((lowerBound - 1).key() + lowerBound.key()))
        return (lowerBound - 1).value();
    else
        return lowerBound.value();
}
