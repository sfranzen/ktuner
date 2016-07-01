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

#include <QQuickWidget>
#include <QQmlContext>
#include <QApplication>
#include <QAction>

#include <KDeclarative/KDeclarative>
#include <KActionCollection>
#include <KStandardAction>
#include <KLocalizedString>

MainWindow::MainWindow(QWidget* parent)
    : KXmlGuiWindow(parent)
    , m_tuner(new KTuner(this))
    , m_view(new QQuickWidget(this))
{
    setCentralWidget(m_view);
    KDeclarative::KDeclarative decl;
    decl.setDeclarativeEngine(m_view->engine());
    decl.setupBindings();
    QQmlContext* ctxt = m_view->engine()->rootContext();
    ctxt->setContextProperty(QStringLiteral("tuner"), m_tuner);
    m_view->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_view->setSource(QUrl("qrc:/main.qml"));
    setupActions();
    setupGUI();
}

void MainWindow::setupActions()
{
    KStandardAction::quit(qApp, &QApplication::quit, actionCollection());
    QAction* showSpectrum = new QAction(this);
    showSpectrum->setText(i18n("&Show Spectrum"));
    showSpectrum->setIcon(QIcon::fromTheme("view-statistics"));
    actionCollection()->addAction("showSpectrum", showSpectrum);
}
