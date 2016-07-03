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
#include "analysisresult.h"
#include "note.h"
#include "mainwindow.h"
#include "version.h"

#include <QApplication>
#include <QtQml>

#include <KAboutData>
#include <KLocalizedString>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    qmlRegisterType<Analyzer>("org.kde.ktuner", 1, 0, "Analyzer");
    qmlRegisterType<AnalysisResult>("org.kde.ktuner", 1, 0, "Result");
    qmlRegisterType<Note>("org.kde.ktuner", 1, 0, "Note");

    KLocalizedString::setApplicationDomain("ktuner");
    KAboutData about(
        QStringLiteral("ktuner"), // componentName
        i18n("kTuner"), // displayName
        QStringLiteral(PROJECT_VERSION), // version
        i18n("An instrument tuner with a simple, clear user interface."), // shortDescription
        KAboutLicense::GPL_V3,
        i18n("© 2016"), // copyrightStatement
        Q_NULLPTR, // otherText
        QStringLiteral("https://github.com/sfranzen/ktuner") // homePageAddress
    );
    about.addAuthor(i18n("Steven Franzen"), i18n("Development"), QStringLiteral("sfranzen85@gmail.com"),
                        QStringLiteral("https://github.com/sfranzen/ktuner"));
    KAboutData::setApplicationData(about);

    MainWindow* window = new MainWindow();
    window->show();

    return app.exec();
}
