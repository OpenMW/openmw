#include <iostream>

#include <QTranslator>
#include <QTextCodec>
#include <QDir>

#ifdef MAC_OS_X_VERSION_MIN_REQUIRED
#undef MAC_OS_X_VERSION_MIN_REQUIRED
// We need to do this because of Qt: https://bugreports.qt-project.org/browse/QTBUG-22154
#define MAC_OS_X_VERSION_MIN_REQUIRED __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__
#endif // MAC_OS_X_VERSION_MIN_REQUIRED

#include "maindialog.hpp"

int main(int argc, char *argv[])
{
    try
    {
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

        return exitCode;
    }
    catch (std::exception& e)
    {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 0;
    }
}
