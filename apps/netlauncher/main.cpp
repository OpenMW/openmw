#include <QApplication>
#include "Main.hpp"
#include "NetController.hpp"

int main(int argc, char *argv[])
{
    // initialize resources, if needed
    // Q_INIT_RESOURCE(resfile);

    NetController::Create();
    atexit(NetController::Destroy);
    QApplication app(argc, argv);
    Main d;
    d.show();
    // create and show your widgets here
    return app.exec();
}