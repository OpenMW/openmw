
#include "newgame.hpp"

#include <QApplication>
#include <QDesktopWidget>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>

#include "filewidget.hpp"

CSVDoc::NewGameDialogue::NewGameDialogue()
{
    setWindowTitle ("Create New Game");

    QVBoxLayout *layout = new QVBoxLayout (this);

    mFileWidget = new FileWidget (this);
    mFileWidget->setType (false);

    layout->addWidget (mFileWidget, 1);

    QDialogButtonBox *buttons = new QDialogButtonBox (this);

    mCreate = new QPushButton ("Create", this);
    mCreate->setDefault (true);
    mCreate->setEnabled (false);

    buttons->addButton (mCreate, QDialogButtonBox::AcceptRole);

    QPushButton *cancel = new QPushButton ("Cancel", this);

    buttons->addButton (cancel, QDialogButtonBox::RejectRole);

    layout->addWidget (buttons);

    setLayout (layout);

    connect (mFileWidget, SIGNAL (stateChanged (bool)), this, SLOT (stateChanged (bool)));
    connect (mCreate, SIGNAL (clicked()), this, SLOT (create()));
    connect (cancel, SIGNAL (clicked()), this, SLOT (reject()));

    QRect scr = QApplication::desktop()->screenGeometry();
    QRect rect = geometry();
    move (scr.center().x() - rect.center().x(), scr.center().y() - rect.center().y());
}

void CSVDoc::NewGameDialogue::stateChanged (bool valid)
{
    mCreate->setEnabled (valid);
}

void CSVDoc::NewGameDialogue::create()
{
    emit createRequest (mFileWidget->getName());
}
