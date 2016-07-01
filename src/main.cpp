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

#include <QApplication>
#include <QtQml>

#include <KDeclarative/KDeclarative>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QQmlApplicationEngine engine;
    KTuner tuner;
    engine.rootContext()->setContextProperty("tuner", &tuner);
//     qmlRegisterType<KTuner>("org.kde.ktuner", 1, 0, "KTuner");
    qmlRegisterType<Analyzer>("org.kde.ktuner", 1, 0, "Analyzer");
    qmlRegisterType<AnalysisResult>("org.kde.ktuner", 1, 0, "Result");
    qmlRegisterType<Note>("org.kde.ktuner", 1, 0, "Note");
    KDeclarative::KDeclarative decl;
    decl.setDeclarativeEngine(&engine);
    decl.setupBindings();
    engine.load(QUrl("qrc:/main.qml"));
    
    if (engine.rootObjects().isEmpty())
        return -1;
    return app.exec();
}

