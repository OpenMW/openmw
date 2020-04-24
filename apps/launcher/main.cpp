#include <iostream>

#include <QApplication>
#include <QTranslator>
#include <QTextCodec>
#include <QDir>
#include <QDebug>

#ifdef MAC_OS_X_VERSION_MIN_REQUIRED
#undef MAC_OS_X_VERSION_MIN_REQUIRED
// We need to do this because of Qt: https://bugreports.qt-project.org/browse/QTBUG-22154
#define MAC_OS_X_VERSION_MIN_REQUIRED __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__
#endif // MAC_OS_X_VERSION_MIN_REQUIRED

#include "maindialog.hpp"
#include "sdlinit.hpp"

int main(int argc, char *argv[])
{
    try
    {
// Note: we should init SDL2 before Qt4 to avoid crashes on Linux,
// but we should init SDL2 after Qt5 to avoid input issues on MacOS X.
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
        initSDL();
#endif

        QApplication app(argc, argv);

        // Internationalization 
        QString locale = QLocale::system().name().section('_', 0, 0);

        QTranslator appTranslator;
        appTranslator.load(":/translations/" + locale + ".qm");
        app.installTranslator(&appTranslator);

        // Now we make sure the current dir is set to application path
        QDir dir(QCoreApplication::applicationDirPath());

        QDir::setCurrent(dir.absolutePath());

        Launcher::MainDialog mainWin;

        Launcher::FirstRunDialogResult result = mainWin.showFirstRunDialog();
        if (result == Launcher::FirstRunDialogResultFailure)
            return 0;

        if (result == Launcher::FirstRunDialogResultContinue)
            mainWin.show();

        int exitCode = app.exec();

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
        // Disconnect from SDL processes
        quitSDL();
#endif

        return exitCode;
    }
    catch (std::exception& e)
    {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 0;
    }
}
