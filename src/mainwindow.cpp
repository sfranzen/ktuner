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
#include <QStatusBar>
#include <QQmlEngine>

#include <KDeclarative/KDeclarative>
#include <KDeclarative/ConfigPropertyMap>
#include <KActionCollection>
#include <KStandardAction>
#include <KLocalizedString>
#include <KConfigDialog>

using namespace KDeclarative;

MainWindow::MainWindow(QWidget* parent)
    : KXmlGuiWindow(parent)
    , m_tuner(new KTuner(this))
    , m_engine(new QQmlEngine(this))
{
    class KDeclarative decl;
    decl.setDeclarativeEngine(m_engine);
    decl.setupEngine(m_engine);
    decl.setupContext();
    ConfigPropertyMap *config = new ConfigPropertyMap(KTunerConfig::self(), this);
    m_engine->rootContext()->setContextProperty(QStringLiteral("tuner"), m_tuner);
    m_engine->rootContext()->setContextProperty(QStringLiteral("config"), config);

    auto *tunerView = new QQuickWidget(m_engine, this);
    tunerView->setResizeMode(QQuickWidget::SizeRootObjectToView);
    tunerView->setSource(QUrl("qrc:/TunerView.qml"));
    setCentralWidget(tunerView);

    connect(m_tuner->analyzer(), &Analyzer::stateChanged, this, &MainWindow::handleAnalyzerState);

    setupDockWidgets();
    setupActions();
    setupGUI();
}

void MainWindow::handleAnalyzerState(Analyzer::State state)
{
    QString message;
    switch(state) {
    case Analyzer::Loading:
        message = i18n("Loading settings...");
        break;
    case Analyzer::CalibratingFilter:
        message = i18n("Calibrating noise filter...");
        break;
    case Analyzer::Processing:
        message = i18n("Processing...");
        break;
    default:
        return;
    }
    statusBar()->showMessage(message);
    if (state == Analyzer::Loading || state == Analyzer::CalibratingFilter) {
        actionCollection()->action("calibrateNoiseFilter")->setDisabled(true);
        actionCollection()->action("enableNoiseFilter")->setDisabled(true);
    } else {
        actionCollection()->action("calibrateNoiseFilter")->setEnabled(KTunerConfig::enableNoiseFilter());
        actionCollection()->action("enableNoiseFilter")->setEnabled(true);
    }
}

void MainWindow::setupActions()
{
    KStandardAction::quit(qApp, &QApplication::quit, actionCollection());
    QAction *preferences = KStandardAction::preferences(this, &MainWindow::showConfig, actionCollection());
    actionCollection()->addAction("preferences", preferences);
    KStandardAction::preferences(this, &MainWindow::showConfig, actionCollection());

    QAction *showSpectrum = m_spectrumView->toggleViewAction();
    showSpectrum->setText(i18n("&Show Spectrum"));
    showSpectrum->setIcon(QIcon::fromTheme("view-statistics"));
    actionCollection()->addAction("showSpectrum", showSpectrum);

    QAction *showAutocorrelation = m_autocorrelationView->toggleViewAction();
    showAutocorrelation->setText(i18n("Show &Autocorrelation"));
    showAutocorrelation->setIcon(QIcon::fromTheme("pathshape"));
    actionCollection()->addAction("showAutocorrelation", showAutocorrelation);

    QAction *calibrateNoiseFilter = new QAction(this);
    calibrateNoiseFilter->setText(i18n("&Recalibrate Noise Filter"));
    calibrateNoiseFilter->setIcon(QIcon::fromTheme("chronometer-reset"));
    calibrateNoiseFilter->setEnabled(KTunerConfig::enableNoiseFilter());
    actionCollection()->addAction("calibrateNoiseFilter", calibrateNoiseFilter);
    connect(calibrateNoiseFilter, &QAction::triggered, m_tuner->analyzer(), &Analyzer::resetFilter);

    QAction *enableFilter = new QAction(this);
    enableFilter->setText(i18n("&Enable Noise Filter"));
    enableFilter->setIcon(QIcon::fromTheme("kt-add-filters"));
    enableFilter->setCheckable(true);
    enableFilter->setChecked(KTunerConfig::enableNoiseFilter());
    actionCollection()->addAction("enableNoiseFilter", enableFilter);
    connect(enableFilter, &QAction::toggled, KTunerConfig::self(), &KTunerConfig::setEnableNoiseFilter);
    connect(enableFilter, &QAction::toggled, KTunerConfig::self(), &KTunerConfig::save);
}

void MainWindow::setupDockWidgets()
{
    auto *widget = new QQuickWidget(m_engine, this);
    widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    widget->setSource(QUrl("qrc:/SpectrumChart.qml"));
    m_spectrumView = new QDockWidget(i18n("Spectrum Viewer"), this);
    m_spectrumView->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_spectrumView->setWidget(widget);
    m_spectrumView->setObjectName(i18n("Spectrum Viewer"));
    m_spectrumView->hide();
    addDockWidget(Qt::RightDockWidgetArea, m_spectrumView);

    widget = new QQuickWidget(m_engine, this);
    widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    widget->setSource(QUrl("qrc:/AutocorrelationChart.qml"));
    m_autocorrelationView = new QDockWidget(i18n("Autocorrelation Viewer"), this);
    m_autocorrelationView->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_autocorrelationView->setWidget(widget);
    m_autocorrelationView->setObjectName(i18n("Autocorrelation Viewer"));
    m_autocorrelationView->hide();
    addDockWidget(Qt::RightDockWidgetArea, m_autocorrelationView);
}

void MainWindow::showConfig()
{
    if (KConfigDialog::showDialog("ktunerconfig"))
        return;

    KTunerConfigDialog *dialog = new KTunerConfigDialog(this);
    dialog->show();
}
