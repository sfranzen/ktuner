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

