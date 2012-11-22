
#include "view.hpp"

#include <iostream>

#include <QCloseEvent>

#include "viewmanager.hpp"

void CSVDoc::View::closeEvent (QCloseEvent *event)
{
    if (!mViewManager.closeRequest (this))
        event->ignore();
}

CSVDoc::View::View (ViewManager& viewManager, CSMDoc::Document *document)
: mViewManager (viewManager), mDocument (document)
{
    resize (200, 200);
    setWindowTitle ("New Document");
}

const CSMDoc::Document *CSVDoc::View::getDocument() const
{
        return mDocument;
}

CSMDoc::Document *CSVDoc::View::getDocument()
{
        return mDocument;
}