
#include "editor.hpp"

#include <exception>
#include <iostream>

#include <QtGui/QApplication>
#include <QIcon>

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
    Application mApplication (argc, argv);

    mApplication.setWindowIcon (QIcon (":./opencs.png"));

    CS::Editor editor;

    return editor.run();
}
