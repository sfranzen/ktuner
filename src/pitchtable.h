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

#ifndef PITCHTABLE_H
#define PITCHTABLE_H

#include "note.h"

#include <QtCore/QMap>

enum class PitchNotation {
    Western
};

/* A table of notes and their fundamental frequencies in twelve-tone equal
 * temperament.
 */
class PitchTable
{    
public:
    PitchTable(qreal concert_A4 = 440.0, PitchNotation notation = PitchNotation::Western);
    ~PitchTable();
    qreal C0() const;
    Note* closestNote(const qreal freq) const;
    
private:
    QStringList m_pitchClasses;
    PitchNotation m_notation;
    qreal m_concert_A4;
    qreal m_C0;
    QMap<qreal, Note*> m_table;
};

#endif // PITCHTABLE_H
