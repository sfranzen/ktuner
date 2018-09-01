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

#include "note.h"
#include "pitchtable.h"
#include "spectrum.h"

#include <QtGlobal>
#include <QObject>
#include <QAudio>
#include <QAudioFormat>
#include <QByteArray>
#include <QVector>
#include <QPointF>

class Analyzer;
class AnalysisResult;
class QIODevice;
class QAudioInput;
namespace QtCharts {
    class QXYSeries;
}

// QT_CHARTS_USE_NAMESPACE

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
    Q_PROPERTY(AnalysisResult* result READ result NOTIFY newResult)

public:
    explicit KTuner(QObject* parent = 0);
    ~KTuner();
    Analyzer* analyzer() const { return m_analyzer; }
    AnalysisResult* result() const { return m_result; }

signals:
    void newResult(AnalysisResult *result);

public slots:
    void updateSpectrum(QtCharts::QXYSeries *series) const;
    void updateAutocorrelation(QtCharts::QXYSeries *series) const;

private slots:
    void loadConfig();
    void processAudioData();
    void processAnalysis(const Spectrum harmonics, const Spectrum spectrum, const Spectrum autocorrelation, Tone snacPeak);
    void onStateChanged(QAudio::State newState) const;

private:
    QAudioFormat m_format;
    QAudioInput *m_audio;
    QIODevice *m_device;
    QByteArray m_buffer;
    int m_bufferPosition;
    qreal m_segmentOverlap;
    Analyzer *m_analyzer;
    AnalysisResult *m_result;
    PitchTable m_pitchTable;
    QVector<QVector<QPointF>> m_seriesData;
    QVector<QVector<QPointF>> m_autocorrelationData;
};

#endif // KTUNER_H
