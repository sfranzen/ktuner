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

#ifndef KTUNERCONFIGDIALOG_H
#define KTUNERCONFIGDIALOG_H

#include <KConfigDialog>

namespace Ui {
    class AnalysisSettings;
    class AudioSettings;
    class GuiSettings;
    class TuningSettings;
}

class KTunerConfigDialog : public KConfigDialog
{
    Q_OBJECT
public:
    explicit KTunerConfigDialog(QWidget *parent = 0);
    void updateSettings() override;
    bool isDefault() override;
    bool hasChanged() override;

protected slots:
    void updateWidgets() override;
    void updateWidgetsDefault() override;

private slots:
    void fetchDeviceCapabilities(int index);
    void setModified();

private:
    bool m_modified;
    Ui::AnalysisSettings *m_analysisSettings;
    Ui::AudioSettings *m_audioSettings;
    Ui::GuiSettings *m_guiSettings;
    Ui::TuningSettings *m_tuningSettings;
};

#endif // KTUNERCONFIGDIALOG_H
