#include "view.hpp"

#include <sstream>
#include <stdexcept>

#include <QCloseEvent>
#include <QMenuBar>
#include <QMdiArea>
#include <QDockWidget>
#include <QtGui/QApplication>

#include "../../model/doc/document.hpp"

#include "../world/subviews.hpp"

#include "../tools/subviews.hpp"

#include "viewmanager.hpp"
#include "operations.hpp"
#include "subview.hpp"

void CSVDoc::View::closeEvent (QCloseEvent *event)
{
    if (!mViewManager.closeRequest (this))
        event->ignore();
}

void CSVDoc::View::setupFileMenu()
{
    QMenu *file = menuBar()->addMenu (tr ("&File"));

    QAction *new_ = new QAction (tr ("New"), this);
    connect (new_, SIGNAL (triggered()), this, SIGNAL (newDocumentRequest()));
    file->addAction (new_);

    QAction *open = new QAction (tr ("&Open"), this);
    connect (open, SIGNAL (triggered()), this, SIGNAL (loadDocumentRequest()));
    file->addAction (open);

    mSave = new QAction (tr ("&Save"), this);
    connect (mSave, SIGNAL (triggered()), this, SLOT (save()));
    file->addAction (mSave);

    QAction *close = new QAction (tr ("&Close"), this);
    connect (close, SIGNAL (triggered()), this, SLOT (close()));
    file->addAction(close);

    QAction *exit = new QAction (tr ("&Exit"), this);
    connect (exit, SIGNAL (triggered()), this, SLOT (exit()));
    connect (this, SIGNAL(exitApplicationRequest(CSVDoc::View *)), &mViewManager, SLOT(exitApplication(CSVDoc::View *)));

    file->addAction(exit);
}

void CSVDoc::View::setupEditMenu()
{
    QMenu *edit = menuBar()->addMenu (tr ("&Edit"));

    mUndo = mDocument->getUndoStack().createUndoAction (this, tr("&Undo"));
    mUndo->setShortcuts (QKeySequence::Undo);
    edit->addAction (mUndo);

    mRedo= mDocument->getUndoStack().createRedoAction (this, tr("&Redo"));
    mRedo->setShortcuts (QKeySequence::Redo);
    edit->addAction (mRedo);
}

void CSVDoc::View::setupViewMenu()
{
    QMenu *view = menuBar()->addMenu (tr ("&View"));

    QAction *newWindow = new QAction (tr ("&New View"), this);
    connect (newWindow, SIGNAL (triggered()), this, SLOT (newView()));
    view->addAction (newWindow);
}

void CSVDoc::View::setupWorldMenu()
{
    QMenu *world = menuBar()->addMenu (tr ("&World"));

    QAction *globals = new QAction (tr ("Globals"), this);
    connect (globals, SIGNAL (triggered()), this, SLOT (addGlobalsSubView()));
    world->addAction (globals);

    QAction *gmsts = new QAction (tr ("Game settings"), this);
    connect (gmsts, SIGNAL (triggered()), this, SLOT (addGmstsSubView()));
    world->addAction (gmsts);

    QAction *skills = new QAction (tr ("Skills"), this);
    connect (skills, SIGNAL (triggered()), this, SLOT (addSkillsSubView()));
    world->addAction (skills);

    mVerify = new QAction (tr ("&Verify"), this);
    connect (mVerify, SIGNAL (triggered()), this, SLOT (verify()));
    world->addAction (mVerify);
}

void CSVDoc::View::setupUi()
{
    setupFileMenu();
    setupEditMenu();
    setupViewMenu();
    setupWorldMenu();
}

void CSVDoc::View::updateTitle()
{
    std::ostringstream stream;

    stream << mDocument->getName();

    if (mDocument->getState() & CSMDoc::State_Modified)
            stream << " *";

    if (mViewTotal>1)
        stream << " [" << (mViewIndex+1) << "/" << mViewTotal << "]";

    setWindowTitle (stream.str().c_str());
}

void CSVDoc::View::updateActions()
{
    bool editing = !(mDocument->getState() & CSMDoc::State_Locked);

    for (std::vector<QAction *>::iterator iter (mEditingActions.begin()); iter!=mEditingActions.end(); ++iter)
        (*iter)->setEnabled (editing);

    mUndo->setEnabled (editing & mDocument->getUndoStack().canUndo());
    mRedo->setEnabled (editing & mDocument->getUndoStack().canRedo());

    mSave->setEnabled (!(mDocument->getState() & CSMDoc::State_Saving));
    mVerify->setEnabled (!(mDocument->getState() & CSMDoc::State_Verifying));
}

CSVDoc::View::View (ViewManager& viewManager, CSMDoc::Document *document, int totalViews)
    : mViewManager (viewManager), mDocument (document), mViewIndex (totalViews-1),
      mViewTotal (totalViews)
{
    resize (300, 300); /// \todo get default size from settings and set reasonable minimal size

    mSubViewWindow.setDockOptions (QMainWindow::AllowNestedDocks);

    setCentralWidget (&mSubViewWindow);

    mOperations = new Operations;
    addDockWidget (Qt::BottomDockWidgetArea, mOperations);

    updateTitle();

    setupUi();

    CSVWorld::addSubViewFactories (mSubViewFactory);
    CSVTools::addSubViewFactories (mSubViewFactory);

    connect (mOperations, SIGNAL (abortOperation (int)), this, SLOT (abortOperation (int)));
}

CSVDoc::View::~View()
{
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
    updateActions();

    static const int operations[] =
    {
        CSMDoc::State_Saving, CSMDoc::State_Verifying,
        -1 // end marker
    };

    int state = mDocument->getState() ;

    for (int i=0; operations[i]!=-1; ++i)
        if (!(state & operations[i]))
           mOperations->quitOperation (operations[i]);

    QList<CSVDoc::SubView *> subViews = findChildren<CSVDoc::SubView *>();

    for (QList<CSVDoc::SubView *>::iterator iter (subViews.begin()); iter!=subViews.end(); ++iter)
        (*iter)->setEditLock (state & CSMDoc::State_Locked);
}

void CSVDoc::View::updateProgress (int current, int max, int type, int threads)
{
    mOperations->setProgress (current, max, type, threads);
}

void CSVDoc::View::addSubView (const CSMWorld::UniversalId& id)
{
    /// \todo add an user setting for limiting the number of sub views per top level view. Automatically open a new top level view if this
    /// number is exceeded

    /// \todo if the sub view limit setting is one, the sub view title bar should be hidden and the text in the main title bar adjusted
    /// accordingly

    /// \todo add an user setting to reuse sub views (on a per document basis or on a per top level view basis)

    SubView *view = mSubViewFactory.makeSubView (id, *mDocument);
    mSubViewWindow.addDockWidget (Qt::TopDockWidgetArea, view);

    connect (view, SIGNAL (focusId (const CSMWorld::UniversalId&)), this,
        SLOT (addSubView (const CSMWorld::UniversalId&)));

    view->show();
}

void CSVDoc::View::newView()
{
    mViewManager.addView (mDocument);
}

void CSVDoc::View::save()
{
    mDocument->save();
}

void CSVDoc::View::verify()
{
    addSubView (mDocument->verify());
}

void CSVDoc::View::addGlobalsSubView()
{
    addSubView (CSMWorld::UniversalId::Type_Globals);
}

void CSVDoc::View::addGmstsSubView()
{
    addSubView (CSMWorld::UniversalId::Type_Gmsts);
}

void CSVDoc::View::addSkillsSubView()
{
    addSubView (CSMWorld::UniversalId::Type_Skills);
}

void CSVDoc::View::abortOperation (int type)
{
    mDocument->abortOperation (type);
    updateActions();
}

CSVDoc::Operations *CSVDoc::View::getOperations() const
{
    return mOperations;
}

void CSVDoc::View::exit()
{
    emit exitApplicationRequest (this);
}
