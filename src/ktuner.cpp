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
#include "ktunerconfig.h"

#include <QtMultimedia>
#include <QtCharts/QXYSeries>
#include <QAudioBuffer>

KTuner::KTuner(QObject* parent)
    : QObject(parent)
    , m_audio(Q_NULLPTR)
    , m_bufferPosition(0)
    , m_thread(this)
    , m_analyzer(new Analyzer(this))
    , m_result(new AnalysisResult(this))
    , m_pitchTable()
{
    loadConfig();
    connect(KTunerConfig::self(), &KTunerConfig::configChanged, this, &KTuner::loadConfig);
    connect(m_analyzer, &Analyzer::done, this, &KTuner::processAnalysis);
}

KTuner::~KTuner()
{
    m_analyzer->deleteLater();
    m_thread.quit();
    m_thread.wait();
    m_audio->stop();
    m_audio->disconnect();
}

void KTuner::loadConfig()
{
    m_segmentOverlap = KTunerConfig::segmentOverlap();

    // Set up and verify the audio format we want
    m_format.setSampleRate(KTunerConfig::sampleRate());
    m_format.setSampleSize(KTunerConfig::sampleSize());
    m_format.setChannelCount(1);
    m_format.setCodec("audio/pcm");
    m_format.setByteOrder(QAudioFormat::LittleEndian);
    m_format.setSampleType(QAudioFormat::SignedInt);

    m_bufferLength = KTunerConfig::segmentLength() * KTunerConfig::sampleSize() / 8;
    m_buffer.fill(0, m_bufferLength);
    m_bufferPosition = 0;

    QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
    foreach (const QAudioDeviceInfo &i, QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
        if (i.deviceName() == KTunerConfig::device()) {
            info = i;
            break;
        }

    if (!info.isFormatSupported(m_format)) {
        qWarning() << "Default audio format not supported. Trying nearest match.";
        m_format = info.nearestFormat(m_format);
    }

    // Set up audio input
    if (m_audio) {
        m_audio->stop();
        delete m_audio;
    }
    m_audio = new QAudioInput(info, m_format, this);
    m_audio->setNotifyInterval(500); // in milliseconds
    m_device = m_audio->start();
    connect(m_audio, &QAudioInput::stateChanged, this, &KTuner::onStateChanged);
    connect(m_device, &QIODevice::readyRead, this, &KTuner::processAudioData);
}

void KTuner::processAudioData()
{
    // Read into buffer and send when we have enough data
    const qint64 bytesReady = m_audio->bytesReady();
    const qint64 bytesAvailable = m_buffer.size() - m_bufferPosition;
    const qint64 bytesToRead = qMin(bytesReady, bytesAvailable);
    const qint64 bytesRead = m_device->read(m_buffer.data() + m_bufferPosition, bytesToRead);

    m_bufferPosition += bytesRead;

    if (m_bufferPosition == m_buffer.size()) {
        m_analyzer->doAnalysis(QAudioBuffer(m_buffer, m_format));
        // Keep the overlapping segment length in buffer and position at end
        // for next read
        qint64 overlap = m_bufferLength * (1 - m_segmentOverlap);
        overlap -= overlap % (m_format.sampleSize() / 8);
        QBuffer m_bufferIO(&m_buffer);
        m_bufferIO.open(QIODevice::ReadOnly);
        m_bufferIO.seek(overlap);
        const QByteArray temp = m_bufferIO.readAll();
        m_bufferIO.close();
        m_buffer.replace(0, temp.size(), temp);
        m_bufferPosition = temp.size();
    }
}

void KTuner::processAnalysis(const Spectrum harmonics, const Spectrum spectrum)
{
    // Prepare spectrum and harmonics for display as QXYSeries
    m_seriesData.clear();
    QVector<QPointF> points;
    points.reserve(spectrum.size());
    for (auto t = spectrum.constBegin(); t < spectrum.constEnd(); ++t)
        points.append(QPointF(t->frequency, t->amplitude));
    m_seriesData.append(points);
    points.clear();
    for (auto h = harmonics.constBegin(); h < harmonics.constEnd(); ++h)
        points.append(QPointF(h->frequency, h->amplitude));
    m_seriesData.append(points);

    qreal deviation = 0;
    qreal fundamental = 0;
    qreal maxAmplitude = std::max_element(spectrum.constBegin(), spectrum.constEnd(),
                                          [](const Tone &t1, const Tone &t2){
                                              return t1.amplitude < t2.amplitude;
                                          })->amplitude;;
    Note newNote = Note(0, "-", "");

    if (harmonics != Analyzer::NullResult) {
        fundamental = harmonics.first().frequency;
        newNote = m_pitchTable.closestNote(fundamental);
        // Estimate deviation in cents by linear approximation 2^(n/1200) ~
        // 1 + 0.0005946*n, where 0 <= n <= 100 is the interval expressed in cents
        deviation = (fundamental / newNote.frequency - 1) / 0.0005946;
    }

    m_result->setFrequency(fundamental);
    m_result->setDeviation(deviation);
    m_result->setNote(newNote);
    m_result->setMaxAmplitude(maxAmplitude);
    emit newResult(m_result);
}

void KTuner::updateSpectrum(QAbstractSeries* series)
{
    if (series) {
        static int seriesIndex = 0;
        QXYSeries* xySeries = static_cast<QXYSeries*>(series);
        xySeries->replace(m_seriesData.at(seriesIndex));
        seriesIndex++;
        seriesIndex %= m_seriesData.size();
    }
}

void KTuner::onStateChanged(const QAudio::State newState)
{
    switch (newState) {
    case QAudio::ActiveState:
    case QAudio::IdleState:
    case QAudio::SuspendedState:
        break;
    case QAudio::StoppedState:
        if (m_audio->error() != QAudio::NoError) {
            qDebug() << "Audio device error: " << m_audio->error();
        }
        break;
    default:
        Q_UNREACHABLE();
    }
}
