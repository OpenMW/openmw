#include "editor.hpp"

#include <exception>
#include <string>

#include <QApplication>
#include <QIcon>
#include <QMetaType>

#include <components/debug/debugging.hpp>

#include "model/doc/messages.hpp"
#include "model/world/universalid.hpp"

#ifdef Q_OS_MAC
#include <QDir>
#endif

Q_DECLARE_METATYPE (std::string)

class Application : public QApplication
{
    private:

        bool notify (QObject *receiver, QEvent *event) override
        {
            try
            {
                return QApplication::notify (receiver, event);
            }
            catch (const std::exception& exception)
            {
                Log(Debug::Error) << "An exception has been caught: " << exception.what();
            }

            return false;
        }

    public:

        Application (int& argc, char *argv[]) : QApplication (argc, argv) {}
};

int runApplication(int argc, char *argv[])
{
#ifdef Q_OS_MAC
    setenv("OSG_GL_TEXTURE_STORAGE", "OFF", 0);
#endif

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

    CS::Editor editor(argc, argv);
#ifdef __linux__
    setlocale(LC_NUMERIC,"C");
#endif

    if(!editor.makeIPCServer())
    {
        editor.connectToIPCServer();
        return 0;
    }

    return editor.run();
}


int main(int argc, char *argv[])
{
    return wrapApplication(&runApplication, argc, argv, "OpenMW-CS");
}
