#include "newgame.hpp"

#include <QApplication>
#include <QDesktopWidget>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#include <QScreen>
#endif

#include "filewidget.hpp"
#include "adjusterwidget.hpp"

CSVDoc::NewGameDialogue::NewGameDialogue()
{
    setWindowTitle ("Create New Game");

    QVBoxLayout *layout = new QVBoxLayout (this);

    mFileWidget = new FileWidget (this);
    mFileWidget->setType (false);

    layout->addWidget (mFileWidget, 1);

    mAdjusterWidget = new AdjusterWidget (this);

    layout->addWidget (mAdjusterWidget, 1);

    QDialogButtonBox *buttons = new QDialogButtonBox (this);

    mCreate = new QPushButton ("Create", this);
    mCreate->setDefault (true);
    mCreate->setEnabled (false);

    buttons->addButton (mCreate, QDialogButtonBox::AcceptRole);

    QPushButton *cancel = new QPushButton ("Cancel", this);

    buttons->addButton (cancel, QDialogButtonBox::RejectRole);

    layout->addWidget (buttons);

    setLayout (layout);

    connect (mAdjusterWidget, SIGNAL (stateChanged (bool)), this, SLOT (stateChanged (bool)));
    connect (mCreate, SIGNAL (clicked()), this, SLOT (create()));
    connect (cancel, SIGNAL (clicked()), this, SLOT (reject()));
    connect (mFileWidget, SIGNAL (nameChanged (const QString&, bool)),
        mAdjusterWidget, SLOT (setName (const QString&, bool)));

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    QRect scr = QGuiApplication::primaryScreen()->geometry();
#else
    QRect scr = QApplication::desktop()->screenGeometry();
#endif
    QRect rect = geometry();
    move (scr.center().x() - rect.center().x(), scr.center().y() - rect.center().y());
}

void CSVDoc::NewGameDialogue::setLocalData (const boost::filesystem::path& localData)
{
    mAdjusterWidget->setLocalData (localData);
}

void CSVDoc::NewGameDialogue::stateChanged (bool valid)
{
    mCreate->setEnabled (valid);
}

void CSVDoc::NewGameDialogue::create()
{
    emit createRequest (mAdjusterWidget->getPath());
}

void CSVDoc::NewGameDialogue::reject()
{
    emit cancelCreateGame ();
    QDialog::reject();
}
