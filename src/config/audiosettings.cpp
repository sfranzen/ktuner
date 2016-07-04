/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016  Steven Franzen <email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
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

#include "audiosettings.h"
#include "ktunerconfig.h"

#include <QAudioDeviceInfo>

AudioSettings::AudioSettings(QWidget* parent)
    : QWidget(parent)
{
    m_ui.setupUi(this);
    connect(m_ui.kcfg_Device, SIGNAL(currentIndexChanged(int)), this, SLOT(deviceChanged(int)));
    foreach (const QAudioDeviceInfo &info, QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
        if (!info.supportedCodecs().isEmpty())
            m_ui.kcfg_Device->addItem(info.deviceName(), qVariantFromValue(info));
    }
}

void AudioSettings::deviceChanged(int idx)
{
    const QAudioDeviceInfo info = m_ui.kcfg_Device->itemData(idx).value<QAudioDeviceInfo>();

    m_ui.kcfg_SampleRate->clear();
    foreach (int i, info.supportedSampleRates())
        m_ui.kcfg_SampleRate->addItem(QString::number(i));
    m_ui.kcfg_SampleRate->setCurrentText("22050");

    m_ui.kcfg_BitDepth->clear();
    foreach (int i, info.supportedSampleSizes())
        m_ui.kcfg_BitDepth->addItem(QString::number(i));
}
