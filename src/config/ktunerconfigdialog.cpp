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
#include "ui_audiosettings.h"
#include "ui_analysissettings.h"
#include "ui_guisettings.h"
#include "ui_tuningsettings.h"

#include <QWidget>
#include <QAudioDeviceInfo>
#include <QtMath>

KTunerConfigDialog::KTunerConfigDialog(QWidget *parent)
    : KConfigDialog(parent, "ktunerconfig", KTunerConfig::self())
    , m_modified(false)
    , m_analysisSettings(new Ui::AnalysisSettings)
    , m_audioSettings(new Ui::AudioSettings)
    , m_guiSettings(new Ui::GuiSettings)
    , m_tuningSettings(new Ui::TuningSettings)
{
    QWidget *page = new QWidget;
    m_guiSettings->setupUi(page);
    addPage(page, i18n("Interface"), QStringLiteral("preferences-desktop-font"));

    page = new QWidget;
    m_audioSettings->setupUi(page);
    addPage(page, i18n("Audio"), QStringLiteral("preferences-desktop-sound"));
    connect(m_audioSettings->device, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &KTunerConfigDialog::fetchDeviceCapabilities);
    connect(m_audioSettings->device, QOverload<int>::of(&QComboBox::activated), this, &KTunerConfigDialog::setModified);
    connect(m_audioSettings->sampleRate, QOverload<int>::of(&QComboBox::activated), this, &KTunerConfigDialog::setModified);
    connect(m_audioSettings->sampleSize, QOverload<int>::of(&QComboBox::activated), this, &KTunerConfigDialog::setModified);
    for (const QAudioDeviceInfo &info : QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
        if (!info.supportedCodecs().isEmpty())
            m_audioSettings->device->addItem(info.deviceName(), qVariantFromValue(info));
    }
    const auto index = m_audioSettings->device->findText(KTunerConfig::device());
    m_audioSettings->device->setCurrentIndex(index);

    page = new QWidget;
    m_analysisSettings->setupUi(page);
    addPage(page, i18n("Analysis"), QStringLiteral("view-object-histogram-linear"));
    connect(m_analysisSettings->segmentLength, QOverload<int>::of(&QComboBox::activated), this, &KTunerConfigDialog::setModified);
    // Populate with powers of two for the FFT algorithm
    for (int i = qPow(2,8); i <= qPow(2,15); i *= 2)
        m_analysisSettings->segmentLength->addItem(QString::number(i));
    m_analysisSettings->kcfg_WindowFunction->addItems(QStringList {"Rectangular Window", "Hann Window", "Gaussian Window"});

    page = new QWidget;
    m_tuningSettings->setupUi(page);
    addPage(page, i18n("Tuning"), QStringLiteral("music-note-16th"));
    m_tuningSettings->kcfg_PitchNotation->addItems(QStringList {"Western, using sharps", "Western, using flats"});
}

void KTunerConfigDialog::updateSettings()
{
    KTunerConfig::setDevice(m_audioSettings->device->currentText());
    KTunerConfig::setSampleRate(m_audioSettings->sampleRate->currentText().toInt());
    KTunerConfig::setSampleSize(m_audioSettings->sampleSize->currentText().toInt());

    KTunerConfig::setSegmentLength(m_analysisSettings->segmentLength->currentText().toInt());

    KTunerConfig::self()->save();
    KConfigDialog::settingsChangedSlot();
    m_modified = false;
}

void KTunerConfigDialog::fetchDeviceCapabilities(int index)
{
    const auto info = m_audioSettings->device->itemData(index).value<QAudioDeviceInfo>();

    m_audioSettings->sampleRate->clear();
    for (auto i : info.supportedSampleRates())
        m_audioSettings->sampleRate->addItem(QString::number(i));
    m_audioSettings->sampleRate->setCurrentText(QString::number(KTunerConfig::sampleRate()));

    m_audioSettings->sampleSize->clear();
    for (auto i : info.supportedSampleSizes())
        m_audioSettings->sampleSize->addItem(QString::number(i));
    m_audioSettings->sampleSize->setCurrentText(QString::number(KTunerConfig::sampleSize()));
}

void KTunerConfigDialog::updateWidgets()
{
    m_audioSettings->device->setCurrentText(KTunerConfig::device());
    m_audioSettings->sampleRate->setCurrentText(QString::number(KTunerConfig::sampleRate()));
    m_audioSettings->sampleSize->setCurrentText(QString::number(KTunerConfig::sampleSize()));

    m_analysisSettings->segmentLength->setCurrentText(QString::number(KTunerConfig::segmentLength()));
}

void KTunerConfigDialog::updateWidgetsDefault()
{
    m_audioSettings->device->setCurrentText(KTunerConfig::defaultDeviceValue());
    m_audioSettings->sampleRate->setCurrentText(QString::number(KTunerConfig::defaultSampleRateValue()));
    m_audioSettings->sampleSize->setCurrentText(QString::number(KTunerConfig::defaultSampleSizeValue()));

    m_analysisSettings->segmentLength->setCurrentText(QString::number(KTunerConfig::defaultSegmentLengthValue()));
}

void KTunerConfigDialog::setModified()
{
    m_modified = true;
    KConfigDialog::updateButtons();
}

bool KTunerConfigDialog::isDefault()
{
    if (m_audioSettings->device->currentText() != KTunerConfig::defaultDeviceValue() ||
        m_audioSettings->sampleRate->currentText().toInt() != KTunerConfig::defaultSampleRateValue() ||
        m_audioSettings->sampleSize->currentText().toInt() != KTunerConfig::defaultSampleSizeValue() ||
        m_analysisSettings->segmentLength->currentText().toInt() != KTunerConfig::defaultSegmentLengthValue())
        return false;
    return KConfigDialog::isDefault();
}

bool KTunerConfigDialog::hasChanged()
{
    if (m_modified)
        return true;
    return KConfigDialog::hasChanged();
}
