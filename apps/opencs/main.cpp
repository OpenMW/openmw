
#include "editor.hpp"

#include <exception>
#include <iostream>

#include <QApplication>
#include <QIcon>

#include <components/ogreinit/ogreinit.hpp>

#ifdef Q_OS_MAC
#include <QDir>
#endif

class Application : public QApplication
{
    private:

        bool notify (QObject *receiver, QEvent *event)
        {
            try
            {
                return QApplication::notify (receiver, event);
            }
            catch (const std::exception& exception)
            {
                std::cerr << "An exception has been caught: " << exception.what() << std::endl;
            }

            return false;
        }

    public:

        Application (int& argc, char *argv[]) : QApplication (argc, argv) {}
};

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE (resources);

    // TODO: Ogre startup shouldn't be here, but it currently has to:
    // SceneWidget destructor will delete the created render window, which would be called _after_ Root has shut down :(

    Application mApplication (argc, argv);
    OgreInit::OgreInit ogreInit;
    ogreInit.init("./opencsOgre.log"); // TODO log path?

#ifdef Q_OS_MAC
    QDir dir(QCoreApplication::applicationDirPath());
    if (dir.dirName() == "MacOS") {
        dir.cdUp();
        dir.cdUp();
        dir.cdUp();
    }
    QDir::setCurrent(dir.absolutePath());

    // force Qt to load only LOCAL plugins, don't touch system Qt installation
    QDir pluginsPath(QCoreApplication::applicationDirPath());
    pluginsPath.cdUp();
    pluginsPath.cd("Plugins");

    QStringList libraryPaths;
    libraryPaths << pluginsPath.path() << QCoreApplication::applicationDirPath();
    mApplication.setLibraryPaths(libraryPaths);
#endif

    mApplication.setWindowIcon (QIcon (":./opencs.png"));

    CS::Editor editor;

    if(!editor.makeIPCServer())
    {
        editor.connectToIPCServer();
       // return 0;
    }

    return editor.run();
}
