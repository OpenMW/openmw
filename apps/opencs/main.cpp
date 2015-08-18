#include "editor.hpp"

#include <exception>
#include <iostream>
#include <string>

#include <QApplication>
#include <QIcon>
#include <QMetaType>

#include "model/doc/messages.hpp"

#include "model/world/universalid.hpp"

#ifdef Q_OS_MAC
#include <QDir>
#endif

Q_DECLARE_METATYPE (std::string)

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
    try
    {
        // To allow background thread drawing in OSG
        QApplication::setAttribute(Qt::AA_X11InitThreads, true);

        Q_INIT_RESOURCE (resources);

        qRegisterMetaType<std::string> ("std::string");
        qRegisterMetaType<CSMWorld::UniversalId> ("CSMWorld::UniversalId");
        qRegisterMetaType<CSMDoc::Message> ("CSMDoc::Message");

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

        application.setWindowIcon (QIcon (":./openmw-cs.png"));

        CS::Editor editor;

        if(!editor.makeIPCServer())
        {
            editor.connectToIPCServer();
            return 0;
        }
        return editor.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 0;
    }

}
