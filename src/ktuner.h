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
#include <QXYSeries>

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

public:
    explicit KTuner(QObject* parent = 0);
    ~KTuner();
    Analyzer* analyzer() const { return m_analyzer; }
    AnalysisResult* result() { return m_result; }

signals:
    void newResult(AnalysisResult* result);

public slots:
    void updateSpectrum(QXYSeries* series);
    void updateAutocorrelation(QXYSeries* series);

private slots:
    void onStateChanged(QAudio::State newState);
    void processAudioData();
    void processAnalysis(const Spectrum harmonics, const Spectrum spectrum, const Spectrum autocorrelation);
    void loadConfig();

private:
    QAudioFormat m_format;
    QAudioInput* m_audio;
    QIODevice* m_device;
    QByteArray m_buffer;
    int m_bufferPosition;
    int m_bufferLength;
    qreal m_segmentOverlap;
    QThread m_thread;
    Analyzer* m_analyzer;
    AnalysisResult* m_result;
    PitchTable m_pitchTable;
    QVector<QVector<QPointF>> m_seriesData;
    QVector<QVector<QPointF>> m_autocorrelationData;
};

#endif // KTUNER_H
