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

#include "ktuner.h"
#include "analyzer.h"

#include <QtMultimedia>
#include <QtCharts/QXYSeries>

KTuner::KTuner(QObject* parent)
    : QObject(parent)
    , m_bufferPosition(0)
    , m_segmentOverlap(0.5)
    , m_thread(this)
    , m_pitchTable()
{
    // Set up and verify the audio format we want
    m_format.setSampleRate(22050);
    m_format.setSampleSize(8);
    m_format.setChannelCount(1);
    m_format.setCodec("audio/pcm");
    m_format.setByteOrder(QAudioFormat::LittleEndian);
    m_format.setSampleType(QAudioFormat::SignedInt);

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultInputDevice());
    if (!info.isFormatSupported(m_format)) {
        qWarning() << "Default audio format not supported. Trying nearest match.";
        m_format = info.nearestFormat(m_format);
    }

    // Set up analyzer
    m_analyzer = new Analyzer(this);
    m_result = new AnalysisResult(this);
    connect(m_analyzer, &Analyzer::done, this, &KTuner::processAnalysis);
    connect(m_analyzer, &Analyzer::sampleSizeChanged, this, &KTuner::setArraySizes);
    setArraySizes(m_analyzer->sampleSize());

    // Set up audio input
    m_audio = new QAudioInput(m_format, this);
    m_audio->setNotifyInterval(500); // in milliseconds
    connect(m_audio, &QAudioInput::stateChanged, this, &KTuner::onStateChanged);
    m_device = m_audio->start();
    connect(m_device, &QIODevice::readyRead, this, &KTuner::processAudioData);
//     connect(m_audio, &QAudioInput::notify, this, &KTuner::sendSamples);
//     connect(this, &KTuner::start, m_analyzer, &Analyzer::doAnalysis);
//     m_analyzer->moveToThread(&m_thread);
//     connect(&m_thread, &QThread::finished, m_analyzer, &QObject::deleteLater);
//     m_thread.start();
}

KTuner::~KTuner()
{
    m_analyzer->deleteLater();
    m_thread.quit();
    m_thread.wait();
    m_audio->stop();
    m_audio->disconnect();
}

void KTuner::processAudioData()
{
    // Read into buffer and send when we have enough data
    const qint64 bytesReady = m_audio->bytesReady();
    const qint64 bytesAvailable = m_buffer.size() - m_bufferPosition;
    const qint64 bytesToRead = qMin(bytesReady, bytesAvailable);
    const qint64 bytesRead = m_device->read(m_buffer.data() + m_bufferPosition, bytesToRead);

    m_bufferPosition += bytesRead;

    if (m_bufferPosition == m_buffer.size() && m_analyzer->isReady()) {
        m_analyzer->doAnalysis(m_buffer, m_format);
        // Keep the overlapping segment length in buffer and position at end
        // for next read
        QBuffer m_bufferIO(&m_buffer);
        m_bufferIO.open(QIODevice::ReadOnly);
        m_bufferIO.seek(m_bufferLength * (1 - m_segmentOverlap));
        const QByteArray temp = m_bufferIO.readAll();
        m_bufferIO.close();
        m_buffer.replace(0, temp.size(), temp);
        m_bufferPosition = temp.size();
    }
}

void KTuner::processAnalysis(const qreal frequency, const Spectrum spectrum)
{
    // Prepare spectrum for display as QXYSeries
    m_spectrumPoints.clear();
    m_spectrumPoints.reserve(spectrum.size());
    foreach (const Tone t, spectrum) {
        m_spectrumPoints.append(QPointF(t.frequency, t.amplitude));
    }

    // Estimate deviation in cents by linear approximation 2^(n/1200) ~
    // 1 + 0.0005946*n, where 0 <= n <= 100 is the interval expressed in cents
    Note* newNote = m_pitchTable.closestNote(frequency);
    const qreal deviation = (frequency / newNote->frequency() - 1) / 0.0005946;
    m_result->setFrequency(frequency);
    m_result->setNote(newNote);
    m_result->setDeviation(deviation);
    emit newResult(m_result);
}

void KTuner::updateSpectrum(QAbstractSeries* series)
{
    if (series) {
        QXYSeries* xySeries = static_cast<QXYSeries*>(series);
        xySeries->replace(m_spectrumPoints);
    }
}

void KTuner::onStateChanged(const QAudio::State newState)
{
    if (m_audio->error() != QAudio::NoError) {
        qDebug() << "Audio device error: " << m_audio->error();
    }
    switch (newState) {
    case QAudio::ActiveState:
        break;
    case QAudio::IdleState:
        // Returned directly after successful access
        break;
    case QAudio::SuspendedState:
        break;
    case QAudio::StoppedState:
        break;
    }
}

void KTuner::setArraySizes(quint32 size)
{
    m_bufferLength = size * m_format.sampleSize() / 8;
    m_buffer.fill(0, m_bufferLength);
}

void KTuner::setSegmentOverlap(qreal overlap)
{
    overlap = qBound(0.0, overlap, 1 - 1.0 / m_analyzer->sampleSize());
    if (m_segmentOverlap != overlap) {
        m_segmentOverlap = overlap;
        emit segmentOverlapChanged(overlap);
    }
}

#include "ktuner.moc"
