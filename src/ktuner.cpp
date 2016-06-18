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

#include "ktuner.h"
#include "analyzer.h"

#include <QtMultimedia>
#include <QtCharts/QXYSeries>

KTuner::KTuner(QObject* parent)
    : QObject(parent)
    , m_bufferPosition(0)
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

    // Store up to 10 seconds of audio in the buffer
    m_buffer.resize(m_format.bytesForFrames(4096)); // in microseconds
    m_buffer.fill(0);

    // Set up analyzer
    m_analyzer = new Analyzer(this);
    m_result = new AnalysisResult(this);
    connect(m_analyzer, &Analyzer::done, this, &KTuner::process);
    connect(m_analyzer, &Analyzer::sampleSizeChanged, this, &KTuner::setArraySizes);
    setArraySizes(m_analyzer->sampleSize());

    // Set up audio input
    m_audio = new QAudioInput(m_format, this);
    m_audio->setNotifyInterval(500); // in milliseconds
    connect(m_audio, &QAudioInput::stateChanged, this, &KTuner::onStateChanged);
    m_device = m_audio->start();
    connect(m_device, &QIODevice::readyRead, this, &KTuner::sendSamples);
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

void KTuner::sendSamples()
{
    // Read into buffer and send when we have enough data
    const qint64 bytesReady = m_audio->bytesReady();
    const qint64 bytesAvailable = m_buffer.size() - m_bufferPosition;
    const qint64 bytesToRead = qMin(bytesReady, bytesAvailable);
    const qint64 bytesRead = m_device->read(m_buffer.data() + m_bufferPosition, bytesToRead);

    m_bufferPosition += bytesRead;

    if (m_bufferPosition == m_buffer.size() && m_analyzer->isReady()) {
        m_analyzer->doAnalysis(m_buffer, &m_format);
        m_buffer.fill(0);
        m_bufferPosition = 0;
    }
}

void KTuner::process(const qreal frequency, const Spectrum spectrum)
{
    m_currentSpectrum = spectrum;
    Note* newNote = m_pitchTable.closestNote(frequency);
    // Estimate deviation in cents by linear approximation 2^(n/1200) ~
    // 1 + 0.0005946*n, where 0 <= n <= 100 is the interval expressed in cents
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
        QVector<QPointF> points;
        foreach (const Tone t, m_currentSpectrum) {
            points.append(QPointF(t.frequency, t.amplitude));
        }
        xySeries->replace(points);
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

void KTuner::setArraySizes(uint size)
{
    m_bufferLength = m_format.bytesForFrames(size);
    m_buffer.resize(m_bufferLength);
    m_buffer.fill(0);
}

#include "ktuner.moc"
