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

#include "ktunerconfigdialog.h"
#include "ktunerconfig.h"

#include <QWidget>
#include <QAudioDeviceInfo>

KTunerConfigDialog::KTunerConfigDialog(QWidget* parent)
    : KConfigDialog(parent, "ktunerconfig", KTunerConfig::self())
    , m_modified(false)
{
    QWidget *page1 = new QWidget;
    m_audioSettings.setupUi(page1);
    addPage(page1, i18n("Audio"), QStringLiteral("preferences-desktop-sound"));
    connect(m_audioSettings.device, SIGNAL(currentIndexChanged(int)), SLOT(deviceChanged(int)));
    connect(m_audioSettings.sampleRate, SIGNAL(activated(int)), SLOT(setModified()));
    connect(m_audioSettings.bitDepth, SIGNAL(activated(int)), SLOT(setModified()));
    foreach (const QAudioDeviceInfo &info, QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
        if (!info.supportedCodecs().isEmpty())
            m_audioSettings.device->addItem(info.deviceName(), qVariantFromValue(info));
    }
    const int index = m_audioSettings.device->findText(KTunerConfig::device());
    m_audioSettings.device->setCurrentIndex(qMax(0, index));
}

void KTunerConfigDialog::updateSettings()
{
    if (m_audioSettings.device->currentText() != KTunerConfig::device())
        KTunerConfig::setDevice(m_audioSettings.device->currentText());
    if (m_audioSettings.sampleRate->currentText().toInt() != KTunerConfig::sampleRate())
        KTunerConfig::setSampleRate(m_audioSettings.sampleRate->currentText().toInt());
    if (m_audioSettings.bitDepth->currentText().toInt() != KTunerConfig::bitDepth())
        KTunerConfig::setBitDepth(m_audioSettings.bitDepth->currentText().toInt());

    KTunerConfig::self()->save();
    KConfigDialog::settingsChangedSlot();
    m_modified = false;
}

void KTunerConfigDialog::deviceChanged(int index)
{
    const QAudioDeviceInfo info = m_audioSettings.device->itemData(index).value<QAudioDeviceInfo>();

    m_audioSettings.sampleRate->clear();
    foreach (int i, info.supportedSampleRates())
        m_audioSettings.sampleRate->addItem(QString::number(i));
    m_audioSettings.sampleRate->setCurrentText(QString::number(KTunerConfig::sampleRate()));

    m_audioSettings.bitDepth->clear();
    foreach (int i, info.supportedSampleSizes())
        m_audioSettings.bitDepth->addItem(QString::number(i));
    m_audioSettings.sampleRate->setCurrentText(QString::number(KTunerConfig::bitDepth()));

    setModified();
}

void KTunerConfigDialog::updateWidgets()
{
    m_audioSettings.device->setCurrentText(KTunerConfig::device());
    m_audioSettings.sampleRate->setCurrentText(QString::number(KTunerConfig::sampleRate()));
    m_audioSettings.bitDepth->setCurrentText(QString::number(KTunerConfig::bitDepth()));
}

void KTunerConfigDialog::updateWidgetsDefault()
{
    m_audioSettings.device->setCurrentText(KTunerConfig::defaultDeviceValue());
    m_audioSettings.sampleRate->setCurrentText(QString::number(KTunerConfig::defaultSampleRateValue()));
    m_audioSettings.bitDepth->setCurrentText(QString::number(KTunerConfig::defaultBitDepthValue()));
}

void KTunerConfigDialog::setModified()
{
    m_modified = true;
    KConfigDialog::updateButtons();
}

bool KTunerConfigDialog::isDefault()
{
    bool isDefault = true;
    if (m_audioSettings.device->currentText() != KTunerConfig::defaultDeviceValue())
        isDefault = false;
    if (m_audioSettings.sampleRate->currentText().toInt() != KTunerConfig::defaultSampleRateValue())
        isDefault = false;
    if (m_audioSettings.bitDepth->currentText().toInt() != KTunerConfig::defaultBitDepthValue())
        isDefault = false;
    return isDefault;
}

bool KTunerConfigDialog::hasChanged()
{
    if (m_modified) return true;
    return KConfigDialog::hasChanged();
}
