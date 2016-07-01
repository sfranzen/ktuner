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

#ifndef NOTE_H
#define NOTE_H

#include <QtCore/QObject>

class Note : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal frequency  READ frequency)
    Q_PROPERTY(QString name     READ name)
    Q_PROPERTY(int octave       READ octave)

public:
    Note(qreal frequency = 0.0, QString name = "", int octave = 0);
    
    qreal frequency() const { return m_frequency; }
    int octave() const { return m_octave; }
    QString name() const { return m_name; }
    
private:
    qreal m_frequency;
    QString m_name;
    int m_octave;
};

#endif // NOTE_H
