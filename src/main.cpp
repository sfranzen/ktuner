#include "ktuner.h"
#include "analyzer.h"
#include "analysisresult.h"
#include "note.h"
#include "mainwindow.h"

#include <QApplication>
#include <QtQml>

#include <KDeclarative/KDeclarative>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    qmlRegisterType<Analyzer>("org.kde.ktuner", 1, 0, "Analyzer");
    qmlRegisterType<AnalysisResult>("org.kde.ktuner", 1, 0, "Result");
    qmlRegisterType<Note>("org.kde.ktuner", 1, 0, "Note");

    MainWindow* window = new MainWindow();
    window->show();

    return app.exec();
}

