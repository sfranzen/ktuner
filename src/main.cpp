#include "ktuner.h"
#include "analyzer.h"
#include "analysisresult.h"
#include "note.h"
#include "mainwindow.h"

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
        QStringLiteral("0.1"), // version
        i18n("An instrument tuner with a simple, clear user interface."), // shortDescription
        KAboutLicense::LGPL_V2,
        i18n("© 2016") // copyrightStatement
//         i18n(""), // otherText
//         QStringLiteral(""), // homePageAddress
//         QStringLiteral("") // bugsEmailAddress
    );
    about.addAuthor(i18n("Steven Franzen"), i18n("Development"), QStringLiteral("sfranzen85@gmail.com"),
                        QStringLiteral("https://github.com/sfranzen"));
    KAboutData::setApplicationData(about);

    MainWindow* window = new MainWindow();
    window->show();

    return app.exec();
}
