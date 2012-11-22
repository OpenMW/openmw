
#include "view.hpp"

#include <iostream>

#include <QCloseEvent>
#include <QMenuBar>

#include "viewmanager.hpp"

void CSVDoc::View::closeEvent (QCloseEvent *event)
{
    if (!mViewManager.closeRequest (this))
        event->ignore();
}

void CSVDoc::View::setupUi()
{
    // window menu
    QMenu *view = menuBar()->addMenu (tr ("&View"));

    QAction *newWindow = new QAction (tr ("&New View"), this);
    connect (newWindow, SIGNAL (triggered()), this, SLOT (newView()));

    view->addAction (newWindow);
}

CSVDoc::View::View (ViewManager& viewManager, CSMDoc::Document *document)
: mViewManager (viewManager), mDocument (document)
{
    setCentralWidget (new QWidget);
    resize (200, 200);
    setWindowTitle ("New Document");

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

void CSVDoc::View::newView()
{
    mViewManager.addView (mDocument);
}