#include <QApplication>

#include "maindialog.hpp"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainDialog dialog;
    return dialog.exec();

}

