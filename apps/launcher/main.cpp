#include <QApplication>
#include <QTextCodec>
#include <QDir>

#include "maindialog.hpp"

int main(int argc, char *argv[])
{
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

    return app.exec();
}

