
#include "view.hpp"

#include <sstream>
#include <stdexcept>

#include <QCloseEvent>
#include <QMenuBar>
#include <QMdiArea>

#include "../../model/doc/document.hpp"

#include "../world/subview.hpp"
#include "../world/globals.hpp"

#include "viewmanager.hpp"
#include "operations.hpp"

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

    mSave = new QAction (tr ("&Save"), this);
    connect (mSave, SIGNAL (triggered()), this, SLOT (save()));
    file->addAction (mSave);
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

    if (mDocument->getState() & CSMDoc::Document::State_Modified)
            stream << " *";

    if (mViewTotal>1)
        stream << " [" << (mViewIndex+1) << "/" << mViewTotal << "]";

    setWindowTitle (stream.str().c_str());
}

void CSVDoc::View::updateActions()
{
    bool editing = !(mDocument->getState() & CSMDoc::Document::State_Locked);

    for (std::vector<QAction *>::iterator iter (mEditingActions.begin()); iter!=mEditingActions.end(); ++iter)
        (*iter)->setEnabled (editing);

    mUndo->setEnabled (editing & mDocument->getUndoStack().canUndo());
    mRedo->setEnabled (editing & mDocument->getUndoStack().canRedo());

    mSave->setEnabled (!(mDocument->getState() & CSMDoc::Document::State_Saving));
    mVerify->setEnabled (!(mDocument->getState() & CSMDoc::Document::State_Verifying));
}

CSVDoc::View::View (ViewManager& viewManager, CSMDoc::Document *document, int totalViews)
: mViewManager (viewManager), mDocument (document), mViewIndex (totalViews-1), mViewTotal (totalViews)
{
    setDockOptions (QMainWindow::AllowNestedDocks);

    resize (300, 300); /// \todo get default size from settings and set reasonable minimal size

    mOperations = new Operations;
    addDockWidget (Qt::BottomDockWidgetArea, mOperations);

    updateTitle();

    setupUi();

    mSubViewFactories.insert (std::make_pair (CSMWorld::UniversalId (CSMWorld::UniversalId::Type_Globals),
        new CSVWorld::SubViewFactory<CSVWorld::Globals>()));
}

CSVDoc::View::~View()
{
    for (std::map<CSMWorld::UniversalId, CSVWorld::SubViewFactoryBase *>::iterator iter (mSubViewFactories.begin());
        iter!=mSubViewFactories.end(); ++iter)
        delete iter->second;
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
        CSMDoc::Document::State_Saving, CSMDoc::Document::State_Verifying,
        -1 // end marker
    };

    int state = mDocument->getState() ;

    for (int i=0; operations[i]!=-1; ++i)
        if (!(state & operations[i]))
            mOperations->quitOperation (operations[i]);
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

    std::map<CSMWorld::UniversalId, CSVWorld::SubViewFactoryBase *>::iterator iter = mSubViewFactories.find (id);

    if (iter==mSubViewFactories.end())
        throw std::logic_error ("can't create subview for " + id.toString());

    CSVWorld::SubView *view = iter->second->makeSubView (id, mDocument->getData(), mDocument->getUndoStack());
    addDockWidget (Qt::TopDockWidgetArea, view);
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
    mDocument->verify();
}

void CSVDoc::View::addGlobalsSubView()
{
    addSubView (CSMWorld::UniversalId::Type_Globals);
}