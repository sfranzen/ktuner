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
#include <QAudioBuffer>
#include <QtCharts/QXYSeries>

KTuner::KTuner(QObject* parent)
    : QObject(parent)
    , m_audio(Q_NULLPTR)
    , m_bufferPosition(0)
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
    m_format.setSampleType(QAudioFormat::SignedInt);

    const auto bufferLength = KTunerConfig::segmentLength() * KTunerConfig::sampleSize() / 8;
    m_buffer.fill(0, bufferLength);
    m_bufferPosition = 0;

    QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
    for (const auto &i : QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
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
        m_audio->deleteLater();
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
        qint64 overlap = m_buffer.size() * (1 - m_segmentOverlap);
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

void KTuner::processAnalysis(const Spectrum harmonics, const Spectrum spectrum, const Spectrum autocorrelation)
{
    // Prepare spectrum and harmonics for display as QXYSeries
    m_seriesData.clear();
    m_seriesData.append(spectrum);
    m_seriesData.append(harmonics);
    m_autocorrelationData = autocorrelation;

    qreal deviation = 0;
    qreal fundamental = 0;
    const qreal maxAmplitude = std::max_element(spectrum.constBegin(), spectrum.constEnd())->amplitude;
    Note newNote;

    if (!harmonics.isEmpty()) {
        fundamental = harmonics.first().frequency;
        newNote = m_pitchTable.closestNote(fundamental);
        deviation = 1200 * std::log2(fundamental / newNote.frequency);
    }

    m_result->setFrequency(fundamental);
    m_result->setDeviation(deviation);
    m_result->setNote(newNote);
    m_result->setMaxAmplitude(maxAmplitude);
    emit newResult(m_result);
}

void KTuner::updateSpectrum(QXYSeries* series)
{
    if (series) {
        static int seriesIndex = 0;
        series->replace(m_seriesData.at(seriesIndex));
        seriesIndex = (seriesIndex + 1) % m_seriesData.size();
    }
}

void KTuner::updateAutocorrelation(QXYSeries* series)
{
    if (series)
        series->replace(m_autocorrelationData);
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
