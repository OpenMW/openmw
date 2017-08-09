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
    #ifdef Q_OS_MAC
        setenv("OSG_GL_TEXTURE_STORAGE", "OFF", 0);
    #endif

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
        QDir::setCurrent(dir.absolutePath());
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
