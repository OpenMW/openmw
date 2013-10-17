#include <QApplication>
#include <QTextCodec>
#include <QDir>
#include <QDebug>

#ifdef __APPLE__
// We need to do this because of Qt: https://bugreports.qt-project.org/browse/QTBUG-22154
#define MAC_OS_X_VERSION_MIN_REQUIRED __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__
#endif
#include <SDL.h>

#include "maindialog.hpp"
// SDL workaround
#include "graphicspage.hpp"

int main(int argc, char *argv[])
{
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        qDebug() << "SDL_Init failed: " << QString::fromStdString(SDL_GetError());
        return 0;
    }

    QApplication app(argc, argv);

    // Now we make sure the current dir is set to application path
    QDir dir(QCoreApplication::applicationDirPath());

    #ifdef Q_OS_MAC
    if (dir.dirName() == "MacOS") {
        dir.cdUp();
        dir.cdUp();
        dir.cdUp();
    }

    // force Qt to load only LOCAL plugins, don't touch system Qt installation
    QDir pluginsPath(QCoreApplication::applicationDirPath());
    pluginsPath.cdUp();
    pluginsPath.cd("Plugins");

    QStringList libraryPaths;
    libraryPaths << pluginsPath.path() << QCoreApplication::applicationDirPath();
    app.setLibraryPaths(libraryPaths);
    #endif

    QDir::setCurrent(dir.absolutePath());

    // Support non-latin characters
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));

    MainDialog mainWin;

    if (mainWin.setup()) {
        mainWin.show();
    } else {
        return 0;
    }

    int returnValue = app.exec();
    SDL_Quit();
    return returnValue;
}

