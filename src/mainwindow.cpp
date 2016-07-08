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

#include "mainwindow.h"
#include "ktuner.h"
#include "ktunerconfig.h"
#include "config/ktunerconfigdialog.h"

#include <QQuickWidget>
#include <QDockWidget>
#include <QQmlContext>
#include <QApplication>
#include <QAction>

#include <KDeclarative/KDeclarative>
#include <KDeclarative/ConfigPropertyMap>
#include <KActionCollection>
#include <KStandardAction>
#include <KLocalizedString>
#include <KConfigDialog>

MainWindow::MainWindow(QWidget* parent)
    : KXmlGuiWindow(parent)
    , m_tuner(new KTuner(this))
    , m_dock(new QDockWidget(i18n("Spectrum Viewer"), this))
{
    QQmlEngine *engine = new QQmlEngine;
    KDeclarative::KDeclarative decl;
    decl.setDeclarativeEngine(engine);
    decl.setupBindings();
    KDeclarative::ConfigPropertyMap config(KTunerConfig::self(), this);
    engine->rootContext()->setContextProperty(QStringLiteral("tuner"), m_tuner);
    engine->rootContext()->setContextProperty(QStringLiteral("config"), &config);

    m_tunerView = new QQuickWidget(engine, this);
    m_tunerView->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_tunerView->setSource(QUrl("qrc:/TunerView.qml"));
    setCentralWidget(m_tunerView);

    m_spectrumView = new QQuickWidget(engine, this);
    m_spectrumView->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_spectrumView->setSource(QUrl("qrc:/SpectrumChart.qml"));

    setupDockWidgets();
    setupActions();
    setupGUI();
}

void MainWindow::setupActions()
{
    KStandardAction::quit(qApp, &QApplication::quit, actionCollection());
    QAction *preferences = KStandardAction::preferences(this, &MainWindow::showConfig, actionCollection());
    QAction* showSpectrum = m_dock->toggleViewAction();
    showSpectrum->setText(i18n("&Show Spectrum"));
    showSpectrum->setIcon(QIcon::fromTheme("view-statistics"));
    actionCollection()->addAction("showSpectrum", showSpectrum);
    actionCollection()->addAction("preferences", preferences);
}

void MainWindow::setupDockWidgets()
{
    m_dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_dock->setWidget(m_spectrumView);
    m_dock->setObjectName(QStringLiteral("Spectrum Viewer"));
    m_dock->hide();

    addDockWidget(Qt::RightDockWidgetArea, m_dock);
}

void MainWindow::showConfig()
{
    if (KConfigDialog::showDialog("ktunerconfig"))
        return;

    KTunerConfigDialog *dialog = new KTunerConfigDialog(this);
    dialog->show();
}
