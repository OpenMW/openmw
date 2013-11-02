
#include "editor.hpp"

#include <exception>
#include <iostream>

#include <QApplication>
#include <QIcon>

#include <components/ogreinit/ogreinit.hpp>

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
    OgreInit::OgreInit ogreInit;
    ogreInit.init("./opencsOgre.log"); // TODO log path?

    Application mApplication (argc, argv);

    mApplication.setWindowIcon (QIcon (":./opencs.png"));

    CS::Editor editor;

    if(!editor.makeIPCServer())
    {
        editor.connectToIPCServer();
        return 0;
    }

    return editor.run();
}
