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

#ifndef KTUNER_H
#define KTUNER_H

#include "analyzer.h"
#include "analysisresult.h"
#include "note.h"
#include "pitchtable.h"

#include <QObject>
#include <QThread>
#include <QAudio>
#include <QAudioFormat>
#include <QByteArray>
#include <QtCharts/QAbstractSeries>

class QAudioInput;

QT_CHARTS_USE_NAMESPACE

/* Main tuner class.
 * 
 * The tuner connects to the audio input and directs its Analyzer component to
 * find the fundamental frequency from the given samples. It then looks up this
 * frequency in a table of musical pitches to find the closest match and the 
 * deviation from its exact pitch.
 * 
 * The results are made available via signals to allow the GUI to update itself. 
 * A pointer to the analyzer itself is also available as a QML property to allow 
 * the user to configure its properties.
 */
class KTuner : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Analyzer* analyzer READ analyzer)
    Q_PROPERTY(AnalysisResult* result READ result NOTIFY newResult)
    
    QAudioFormat m_format;
    QAudioInput* m_audio;
    QIODevice* m_device;
    QByteArray m_buffer;
    int m_bufferPosition;
    int m_bufferLength;
    int m_bytesPerSample;
    QThread m_thread;
    Analyzer* m_analyzer;
    AnalysisResult* m_result;
    PitchTable m_pitchTable;
    Spectrum m_currentSpectrum;
    
public:
    KTuner(QObject* parent = 0);
    ~KTuner();
    Analyzer* analyzer() const { return m_analyzer; }
    AnalysisResult* result() { return m_result; }
    
signals:
    void start(const QByteArray input, const QAudioFormat* format);
    void newResult(AnalysisResult* result);

public slots:
    void process(const qreal frequency, const Spectrum spectrum);
    void updateSpectrum(QAbstractSeries* series);
    
private slots:
    void onStateChanged(QAudio::State newState);
    void sendSamples();
    void setArraySizes(uint size);
};

#endif // KTUNER_H
