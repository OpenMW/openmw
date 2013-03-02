
#include "viewmanager.hpp"

#include <map>

#include "../../model/doc/documentmanager.hpp"
#include "../../model/doc/document.hpp"

#include "../world/util.hpp"
#include "../world/enumdelegate.hpp"
#include "../world/vartypedelegate.hpp"

#include "view.hpp"

#include <QDebug>
#include <QMessageBox>


void CSVDoc::ViewManager::updateIndices()
{
    std::map<CSMDoc::Document *, std::pair<int, int> > documents;

    for (std::vector<View *>::const_iterator iter (mViews.begin()); iter!=mViews.end(); ++iter)
    {
        std::map<CSMDoc::Document *, std::pair<int, int> >::iterator document = documents.find ((*iter)->getDocument());

        if (document==documents.end())
            document =
                documents.insert (
                    std::make_pair ((*iter)->getDocument(), std::make_pair (0, countViews ((*iter)->getDocument())))).
                first;

        (*iter)->setIndex (document->second.first++, document->second.second);
    }
}

CSVDoc::ViewManager::ViewManager (CSMDoc::DocumentManager& documentManager)
    : mDocumentManager (documentManager), mCloseMeOnSaveStateChange(0)
{
    mDelegateFactories = new CSVWorld::CommandDelegateFactoryCollection;

    mDelegateFactories->add (CSMWorld::ColumnBase::Display_VarType,
        new CSVWorld::VarTypeDelegateFactory (ESM::VT_None, ESM::VT_String, ESM::VT_Int, ESM::VT_Float));
}

CSVDoc::ViewManager::~ViewManager()
{
    delete mDelegateFactories;

    for (std::vector<View *>::iterator iter (mViews.begin()); iter!=mViews.end(); ++iter)
        delete *iter;
}

CSVDoc::View *CSVDoc::ViewManager::addView (CSMDoc::Document *document)
{
    if (countViews (document)==0)
    {
        // new document
        connect (document, SIGNAL (stateChanged (int, CSMDoc::Document *)),
            this, SLOT (documentStateChanged (int, CSMDoc::Document *)));

        connect (document, SIGNAL (progress (int, int, int, int, CSMDoc::Document *)),
            this, SLOT (progress (int, int, int, int, CSMDoc::Document *)));
    }

   // QMainWindow *mainWindow = new QMainWindow;

    View *view = new View (*this, document, countViews (document)+1); //, mainWindow);

    mViews.push_back (view);

    view->show();

    connect (view, SIGNAL (newDocumentRequest ()), this, SIGNAL (newDocumentRequest()));
    connect (view, SIGNAL (loadDocumentRequest ()), this, SIGNAL (loadDocumentRequest()));

    updateIndices();

    return view;
}

int CSVDoc::ViewManager::countViews (const CSMDoc::Document *document) const
{
    int count = 0;

    for (std::vector<View *>::const_iterator iter (mViews.begin()); iter!=mViews.end(); ++iter)
        if ((*iter)->getDocument()==document)
            ++count;

    return count;
}

bool CSVDoc::ViewManager::closeRequest (View *view)
{
    std::vector<View *>::iterator iter = std::find (mViews.begin(), mViews.end(), view);

    bool continueWithClose = true;

    if (iter!=mViews.end())
    {
        bool last = countViews (view->getDocument())<=1;

        /// \todo check if save is in progress  -> warn user about possible data loss
        /// \todo check if document has not been saved -> return false and start close dialogue

        CSMDoc::Document *document = view->getDocument();

        //notify user of unsaved changes and process response
        if ( document->getState() & CSMDoc::State_Modified)
            continueWithClose = showModifiedDocumentMessageBox (view);

        //notify user of saving in progress
        if ( document->getState() & CSMDoc::State_Saving )
            continueWithClose = showSaveInProgressMessageBox (view);

        qDebug() << "Continue with close? " << continueWithClose;

        if (continueWithClose)
        {
            mViews.erase (iter);

            if (last)
                mDocumentManager.removeDocument (document);
            else
                updateIndices();
        }
    }

    return continueWithClose;
}

bool CSVDoc::ViewManager::showModifiedDocumentMessageBox (View* view)
{
    QMessageBox messageBox;

    messageBox.setText ("The document has been modified.");
    messageBox.setInformativeText ("Do you want to save your changes?");
    messageBox.setStandardButtons (QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    messageBox.setDefaultButton (QMessageBox::Save);

    bool retVal = true;

    switch (messageBox.exec())
    {
        case QMessageBox::Save:
            view->getDocument()->save();
            mCloseMeOnSaveStateChange = view;
            retVal = false;
        break;

        case QMessageBox::Discard:
        break;

        case QMessageBox::Cancel:
            retVal = false;
        break;

        default:
        break;

    }

    return retVal;
}

bool CSVDoc::ViewManager::showSaveInProgressMessageBox (View* view)
{
    QMessageBox messageBox;

    messageBox.setText ("The document is currently being saved.");
    messageBox.setInformativeText("Do you want to abort the save?");
    messageBox.setStandardButtons (QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

    bool retVal = true;

    switch (messageBox.exec())
    {
        case QMessageBox::Yes:    //immediate shutdown
            mCloseMeOnSaveStateChange = 0;
            view->abortOperation(CSMDoc::State_Saving);
        break;

        case QMessageBox::No:    //shutdown after save completes
            mCloseMeOnSaveStateChange = view;
            retVal = false;
        break;

        case QMessageBox::Cancel:  //abort shutdown, allow save to complete
            mCloseMeOnSaveStateChange = 0;
            retVal = false;
        break;

        default:
        break;
    }

    return retVal;
}

void CSVDoc::ViewManager::documentStateChanged (int state, CSMDoc::Document *document)
{
    for (std::vector<View *>::const_iterator iter (mViews.begin()); iter!=mViews.end(); ++iter)
            if ((*iter)->getDocument()==document)
                (*iter)->updateDocumentState();

    if (mPreviousDocumentState & CSMDoc::State_Saving)
        qDebug() << "Last state was saving";
    else
        qDebug() << "Last state was something else";

    //mechanism to shutdown main window after saving operation completes
    if (mCloseMeOnSaveStateChange && (mPreviousDocumentState & CSMDoc::State_Saving))
    {
        mCloseMeOnSaveStateChange->close();
        mCloseMeOnSaveStateChange = 0;
    }

    mPreviousDocumentState = state;
}

void CSVDoc::ViewManager::progress (int current, int max, int type, int threads, CSMDoc::Document *document)
{
    for (std::vector<View *>::const_iterator iter (mViews.begin()); iter!=mViews.end(); ++iter)
            if ((*iter)->getDocument()==document)
                (*iter)->updateProgress (current, max, type, threads);
}
