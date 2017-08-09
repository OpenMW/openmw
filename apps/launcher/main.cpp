#include <iostream>
#include <csignal>

#include <QApplication>
#include <QTextCodec>
#include <QDir>
#include <QDebug>

#ifdef MAC_OS_X_VERSION_MIN_REQUIRED
#undef MAC_OS_X_VERSION_MIN_REQUIRED
// We need to do this because of Qt: https://bugreports.qt-project.org/browse/QTBUG-22154
#define MAC_OS_X_VERSION_MIN_REQUIRED __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__
#endif // MAC_OS_X_VERSION_MIN_REQUIRED

#include <SDL.h>

#include "maindialog.hpp"

int main(int argc, char *argv[])
{
    try
    {
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
        SDL_SetMainReady();
        if (SDL_Init(SDL_INIT_VIDEO) != 0)
        {
            qDebug() << "SDL_Init failed: " << QString::fromUtf8(SDL_GetError());
            return 0;
        }
        signal(SIGINT, SIG_DFL); // We don't want to use the SDL event loop in the launcher,
                                 // so reset SIGINT which SDL wants to redirect to an SDL_Quit event.

        QApplication app(argc, argv);

        // Now we make sure the current dir is set to application path
        QDir dir(QCoreApplication::applicationDirPath());

        QDir::setCurrent(dir.absolutePath());

        Launcher::MainDialog mainWin;

        Launcher::FirstRunDialogResult result = mainWin.showFirstRunDialog();
        if (result == Launcher::FirstRunDialogResultFailure)
            return 0;

        if (result == Launcher::FirstRunDialogResultContinue)
            mainWin.show();

        int returnValue = app.exec();
        SDL_Quit();
        return returnValue;
    }
    catch (std::exception& e)
    {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 0;
    }
}
