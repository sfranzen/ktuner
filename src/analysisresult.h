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

#ifndef ANALYSISRESULT_H
#define ANALYSISRESULT_H

#include "note.h"

#include  <QtCore/QObject>

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
    Q_PROPERTY(qreal    deviation       READ deviation      NOTIFY deviationChanged)
    Q_PROPERTY(qreal    frequency       READ frequency      NOTIFY frequencyChanged)
    Q_PROPERTY(qreal    noteFrequency   READ noteFrequency  NOTIFY noteFrequencyChanged)
    Q_PROPERTY(QString  noteName        READ noteName       NOTIFY noteNameChanged)
    Q_PROPERTY(int      octave          READ octave         NOTIFY octaveChanged)
    
public:
    AnalysisResult(QObject* parent = 0);
    
    qreal deviation() const;
    void setDeviation(qreal deviation);
    qreal frequency() const;
    void setFrequency(qreal frequency);

    void setNote(Note note);
    qreal noteFrequency() const;
    QString noteName() const;
    int octave() const;

signals:
    void deviationChanged(qreal deviation);
    void frequencyChanged(qreal frequency);
    void noteFrequencyChanged(qreal frequency);
    void noteNameChanged(QString name);
    void octaveChanged(int octave);
    
private:
    qreal m_deviation;
    qreal m_frequency;
    Note m_note;
};

#endif // ANALYSISRESULT_H
