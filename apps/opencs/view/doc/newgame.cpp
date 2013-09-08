
#include "newgame.hpp"

#include <QApplication>
#include <QDesktopWidget>

CSVDoc::NewGameDialogue::NewGameDialogue()
{
    setWindowTitle ("Create New Game");

    QRect scr = QApplication::desktop()->screenGeometry();
    QRect rect = geometry();
    move (scr.center().x() - rect.center().x(), scr.center().y() - rect.center().y());
}