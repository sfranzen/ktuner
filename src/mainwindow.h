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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "analyzer.h"

#include <KXmlGuiWindow>

class KTuner;
class QQuickWidget;
class QDockWidget;

class MainWindow : public KXmlGuiWindow
{
public:
    explicit MainWindow(QWidget* parent = 0);

private slots:
    void handleAnalyzerState(Analyzer::State state);

private:
    void setupActions();
    void setupDockWidgets();
    void showConfig();
    KTuner* m_tuner;
    QDockWidget* m_dock;
    QQuickWidget* m_tunerView;
    QQuickWidget* m_spectrumView;
};

#endif // MAINWINDOW_H
