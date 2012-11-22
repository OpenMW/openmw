
#include "view.hpp"

#include <sstream>

#include <QCloseEvent>
#include <QMenuBar>

#include "../../model/doc/document.hpp"

#include "viewmanager.hpp"

void CSVDoc::View::closeEvent (QCloseEvent *event)
{
    if (!mViewManager.closeRequest (this))
        event->ignore();
}

void CSVDoc::View::setupFileMenu()
{
    QMenu *file = menuBar()->addMenu (tr ("&File"));

    QAction *save = new QAction (tr ("&Save"), this);
    connect (save, SIGNAL (triggered()), this, SLOT (save()));
    file->addAction (save);
}

void CSVDoc::View::setupEditMenu()
{
    QMenu *edit = menuBar()->addMenu (tr ("&Edit"));

    QAction *undo = mDocument->getUndoStack().createUndoAction (this, tr("&Undo"));
    undo->setShortcuts (QKeySequence::Undo);
    edit->addAction (undo);

    QAction *redo = mDocument->getUndoStack().createRedoAction (this, tr("&Redo"));
    redo->setShortcuts (QKeySequence::Redo);
    edit->addAction (redo);

    // test
    QAction *test = new QAction (tr ("&Test Command"), this);
    connect (test, SIGNAL (triggered()), this, SLOT (test()));
    edit->addAction (test);
}

void CSVDoc::View::setupViewMenu()
{
    QMenu *view = menuBar()->addMenu (tr ("&View"));

    QAction *newWindow = new QAction (tr ("&New View"), this);
    connect (newWindow, SIGNAL (triggered()), this, SLOT (newView()));
    view->addAction (newWindow);
}

void CSVDoc::View::setupUi()
{
    setupFileMenu();
    setupEditMenu();
    setupViewMenu();
}

void CSVDoc::View::updateTitle()
{
    std::ostringstream stream;

    stream << "New Document ";

    if (mDocument->getState() & CSMDoc::Document::State_Modified)
            stream << " *";

    if (mViewTotal>1)
        stream << " [" << (mViewIndex+1) << "/" << mViewTotal << "]";

    setWindowTitle (stream.str().c_str());
}

CSVDoc::View::View (ViewManager& viewManager, CSMDoc::Document *document, int totalViews)
: mViewManager (viewManager), mDocument (document), mViewIndex (totalViews-1), mViewTotal (totalViews)
{
    setCentralWidget (new QWidget);
    resize (200, 200);

    updateTitle();

    setupUi();
}

const CSMDoc::Document *CSVDoc::View::getDocument() const
{
        return mDocument;
}

CSMDoc::Document *CSVDoc::View::getDocument()
{
        return mDocument;
}

void CSVDoc::View::setIndex (int viewIndex, int totalViews)
{
    mViewIndex = viewIndex;
    mViewTotal = totalViews;
    updateTitle();
}

void CSVDoc::View::updateDocumentState()
{
    updateTitle();
}

void CSVDoc::View::newView()
{
    mViewManager.addView (mDocument);
}

void CSVDoc::View::test()
{
    mDocument->getUndoStack().push (new QUndoCommand());
}

void CSVDoc::View::save()
{
    mDocument->save();
}