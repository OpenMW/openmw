
#include "viewmanager.hpp"

#include <map>

#include "../../model/doc/documentmanager.hpp"
#include "../../model/doc/document.hpp"

#include "../world/util.hpp"
#include "../world/enumdelegate.hpp"
#include "../world/vartypedelegate.hpp"

#include "view.hpp"

#include <QMessageBox>
#include <QPushButton>
#include <QtGui/QApplication>
#include <QDebug>

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
    : mDocumentManager (documentManager), mExitOnSaveStateChange(false), mUserWarned(false)
{
    static const char *sSpecialisations[] =
    {
        "Combat", "Magic", "Stealth", 0
    };

    static const char *sAttributes[] =
    {
        "Strength", "Intelligence", "Willpower", "Agility", "Speed", "Endurance", "Personality",
        "Luck", 0
    };

    mDelegateFactories = new CSVWorld::CommandDelegateFactoryCollection;

    mDelegateFactories->add (CSMWorld::ColumnBase::Display_GmstVarType,
        new CSVWorld::VarTypeDelegateFactory (ESM::VT_None, ESM::VT_String, ESM::VT_Int, ESM::VT_Float));

    mDelegateFactories->add (CSMWorld::ColumnBase::Display_GlobalVarType,
        new CSVWorld::VarTypeDelegateFactory (ESM::VT_Short, ESM::VT_Long, ESM::VT_Float));

    mDelegateFactories->add (CSMWorld::ColumnBase::Display_Specialisation,
        new CSVWorld::EnumDelegateFactory (sSpecialisations));

    mDelegateFactories->add (CSMWorld::ColumnBase::Display_Attribute,
        new CSVWorld::EnumDelegateFactory (sAttributes));
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

    View *view = new View (*this, document, countViews (document)+1);


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

        if (last)
            continueWithClose = notifySaveOnClose (view);
        else
        {
            (*iter)->deleteLater();
            mViews.erase (iter);

            updateIndices();
        }
    }

    return continueWithClose;
}

bool CSVDoc::ViewManager::notifySaveOnClose (CSVDoc::View *view)
{
    bool result = true;
    CSMDoc::Document *document = view->getDocument();

    //notify user of saving in progress
    if ( (document->getState() & CSMDoc::State_Saving) )
        result = showSaveInProgressMessageBox (view);

    //notify user of unsaved changes and process response
    else if ( document->getState() & CSMDoc::State_Modified)
        result = showModifiedDocumentMessageBox (view);

    return result;
}

bool CSVDoc::ViewManager::showModifiedDocumentMessageBox (CSVDoc::View *view)
{
    QMessageBox messageBox;
    CSMDoc::Document *document = view->getDocument();

    messageBox.setText ("The document has been modified.");
    messageBox.setInformativeText ("Do you want to save your changes?");
    messageBox.setStandardButtons (QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    messageBox.setDefaultButton (QMessageBox::Save);

    bool retVal = true;

    connect (this, SIGNAL (closeMessageBox()), &messageBox, SLOT (close()));

    connect (document, SIGNAL (stateChanged (int, CSMDoc::Document *)), this, SLOT (onExitWarningHandler(int, CSMDoc::Document *)));

    mUserWarned = true;
    int response = messageBox.exec();
    mUserWarned = false;

    switch (response)
    {
        case QMessageBox::Save:

            document->save();
            mExitOnSaveStateChange = true;
            retVal = false;
        break;

        case QMessageBox::Discard:

            disconnect (document, SIGNAL (stateChanged (int, CSMDoc::Document *)), this, SLOT (onExitWarningHandler(int, CSMDoc::Document *)));
        break;

        case QMessageBox::Cancel:

            //disconnect to prevent unintended view closures
            disconnect (document, SIGNAL (stateChanged (int, CSMDoc::Document *)), this, SLOT (onExitWarningHandler(int, CSMDoc::Document *)));
            retVal = false;
        break;

        default:
        break;

    }

    return retVal;
}

bool CSVDoc::ViewManager::showSaveInProgressMessageBox (CSVDoc::View *view)
{
    QMessageBox messageBox;
    CSMDoc::Document *document = view->getDocument();

    messageBox.setText ("The document is currently being saved.");
    messageBox.setInformativeText("Do you want to close now and abort saving, or wait until saving has completed?");

    QPushButton* waitButton = messageBox.addButton (tr("Wait"), QMessageBox::YesRole);
    QPushButton* closeButton = messageBox.addButton (tr("Close Now"), QMessageBox::RejectRole);
    QPushButton* cancelButton = messageBox.addButton (tr("Cancel"), QMessageBox::NoRole);

    messageBox.setDefaultButton (waitButton);

    bool retVal = true;

    //Connections shut down message box if operation ends before user makes a decision.
    connect (document, SIGNAL (stateChanged (int, CSMDoc::Document *)), this, SLOT (onExitWarningHandler(int, CSMDoc::Document *)));
    connect (this, SIGNAL (closeMessageBox()), &messageBox, SLOT (close()));

    //set / clear the user warned flag to indicate whether or not the message box is currently active.
    mUserWarned = true;
    messageBox.exec();
    mUserWarned = false;

    //if closed by the warning handler, defaults to the RejectRole button (closeButton)
    if (messageBox.clickedButton() == waitButton)
    {
        //save the View iterator for shutdown after the save operation ends
        mExitOnSaveStateChange = true;
        retVal = false;
    }

    else if (messageBox.clickedButton() == closeButton)
    {
        //disconnect to avoid segmentation fault
        disconnect (document, SIGNAL (stateChanged (int, CSMDoc::Document *)), this, SLOT (onExitWarningHandler(int, CSMDoc::Document *)));

        view->abortOperation(CSMDoc::State_Saving);
        mExitOnSaveStateChange = true;
    }

    else if (messageBox.clickedButton() == cancelButton)
    {
        //abort shutdown, allow save to complete
        //disconnection to prevent unintended view closures
        mExitOnSaveStateChange = false;
        disconnect (document, SIGNAL (stateChanged (int, CSMDoc::Document *)), this, SLOT (onExitWarningHandler(int, CSMDoc::Document *)));
        retVal = false;
    }

    return retVal;
}

void CSVDoc::ViewManager::documentStateChanged (int state, CSMDoc::Document *document)
{
    for (std::vector<View *>::const_iterator iter (mViews.begin()); iter!=mViews.end(); ++iter)
            if ((*iter)->getDocument()==document)
                (*iter)->updateDocumentState();
}

void CSVDoc::ViewManager::progress (int current, int max, int type, int threads, CSMDoc::Document *document)
{
    for (std::vector<View *>::const_iterator iter (mViews.begin()); iter!=mViews.end(); ++iter)
            if ((*iter)->getDocument()==document)
                (*iter)->updateProgress (current, max, type, threads);
}

void CSVDoc::ViewManager::onExitWarningHandler (int state, CSMDoc::Document *document)
{
    if ( !(state & CSMDoc::State_Saving) )
    {
        //if the user is being warned (message box is active), shut down the message box,
        //as there is no save operation currently running
        if ( mUserWarned )
            emit closeMessageBox();

        //otherwise, the user has closed the message box before the save operation ended.
        //exit the application
        else if (mExitOnSaveStateChange)
            QApplication::instance()->exit();
    }
}

void CSVDoc::ViewManager::exitApplication (CSVDoc::View *view)
{
    if (notifySaveOnClose (view))
        QApplication::instance()->exit();
}
