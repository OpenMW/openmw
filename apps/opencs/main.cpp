
#include "editor.hpp"

#include <QtGui/QApplication>

int main(int argc, char *argv[])
{
    QApplication mApplication (argc, argv);

    CS::Editor editor;

    return editor.run();
}