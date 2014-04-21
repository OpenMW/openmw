
#include "editor.hpp"

#include <exception>
#include <iostream>

#include <QApplication>
#include <QIcon>

#include <QDebug>

#include <extern/shiny/Main/Factory.hpp>

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

    OgreInit::OgreInit ogreInit;

    std::auto_ptr<sh::Factory> shinyFactory;

    Application application (argc, argv);

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
    application.setLibraryPaths(libraryPaths);
#endif

    application.setWindowIcon (QIcon (":./opencs.png"));

    CS::Editor editor (ogreInit);

    if(!editor.makeIPCServer())
    {
    	editor.connectToIPCServer();
  //      return 0;
    }

    shinyFactory = editor.setupGraphics();

    return editor.run();
}
