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

#ifndef ANALYSISRESULT_H
#define ANALYSISRESULT_H

#include  <QtCore/QObject>

class Note;

/* Container class for results of a frequency analysis.
 * 
 * This class is used to convey information about the results of the audio 
 * analysis. It contains the measured frequency as well as a the musical note, 
 * identified by frequency, name and octave number, closest to this frequency. 
 * The deviation value is the interval between measurement and note, given in 
 * cents (1/100ths of a semitone).
 */
class AnalysisResult : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal    frequency   READ frequency)
    Q_PROPERTY(Note*    note        READ note)
    Q_PROPERTY(qreal    deviation   READ deviation)
    
    qreal m_frequency;
    Note* m_note;
    qreal m_deviation;
    
public:
    AnalysisResult(QObject* parent = 0) : QObject(parent) {};
    
    qreal frequency() const { return m_frequency; }
    void setFrequency(qreal freq) { m_frequency = freq; }
    Note* note() const { return m_note; }
    void setNote(Note* note) { m_note = note; }
    qreal deviation() const { return m_deviation; }
    void setDeviation(qreal deviation) { m_deviation = deviation; }
};

#endif // ANALYSISRESULT_H
