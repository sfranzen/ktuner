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

#include "analysisresult.h"

AnalysisResult::AnalysisResult(QObject* parent)
    : QObject(parent)
    , m_deviation(0)
    , m_frequency(0)
    , m_note()
{
}

qreal AnalysisResult::deviation() const
{
    return m_deviation;
}

qreal AnalysisResult::frequency() const
{
    return m_frequency;
}

qreal AnalysisResult::maxAmplitude() const
{
    return m_maxAmplitude;
}

qreal AnalysisResult::noteFrequency() const
{
    return m_note.frequency;
}

QString AnalysisResult::octave() const
{
    return m_note.octave;
}

QString AnalysisResult::noteName() const
{
    return m_note.name;
}

void AnalysisResult::setDeviation(qreal deviation)
{
    if (m_deviation != deviation) {
        m_deviation = deviation;
        emit deviationChanged(deviation);
    }
}

void AnalysisResult::setFrequency(qreal frequency)
{
    if (m_frequency != frequency) {
        m_frequency = frequency;
        emit frequencyChanged(frequency);
    }
}

void AnalysisResult::setMaxAmplitude(qreal amplitude)
{
    m_maxAmplitude = amplitude;
}

void AnalysisResult::setNote(Note note)
{
    if (m_note.frequency != note.frequency) {
        m_note = note;
        emit noteFrequencyChanged(note.frequency);
        emit noteNameChanged(note.name);
        emit octaveChanged(note.octave);
    }
}

#include "analysisresult.moc"

