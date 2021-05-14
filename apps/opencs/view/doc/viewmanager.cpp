#include "viewmanager.hpp"

#include <vector>
#include <map>

#include <QApplication>
#include <QDesktopWidget>
#include <QMessageBox>
#include <QPushButton>

#include "../../model/doc/documentmanager.hpp"
#include "../../model/doc/document.hpp"
#include "../../model/world/columns.hpp"
#include "../../model/world/idcompletionmanager.hpp"

#include "../../model/prefs/state.hpp"

#include "../world/util.hpp"
#include "../world/enumdelegate.hpp"
#include "../world/vartypedelegate.hpp"
#include "../world/recordstatusdelegate.hpp"
#include "../world/idtypedelegate.hpp"
#include "../world/idcompletiondelegate.hpp"
#include "../world/colordelegate.hpp"

#include "view.hpp"

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
    mDelegateFactories = new CSVWorld::CommandDelegateFactoryCollection;

    mDelegateFactories->add (CSMWorld::ColumnBase::Display_GmstVarType,
        new CSVWorld::VarTypeDelegateFactory (ESM::VT_None, ESM::VT_String, ESM::VT_Int, ESM::VT_Float));

    mDelegateFactories->add (CSMWorld::ColumnBase::Display_GlobalVarType,
        new CSVWorld::VarTypeDelegateFactory (ESM::VT_Short, ESM::VT_Long, ESM::VT_Float));

    mDelegateFactories->add (CSMWorld::ColumnBase::Display_RecordState,
        new CSVWorld::RecordStatusDelegateFactory());

    mDelegateFactories->add (CSMWorld::ColumnBase::Display_RefRecordType,
        new CSVWorld::IdTypeDelegateFactory());

    mDelegateFactories->add (CSMWorld::ColumnBase::Display_Colour,
        new CSVWorld::ColorDelegateFactory());

    std::vector<CSMWorld::ColumnBase::Display> idCompletionColumns = CSMWorld::IdCompletionManager::getDisplayTypes();
    for (std::vector<CSMWorld::ColumnBase::Display>::const_iterator current = idCompletionColumns.begin();
         current != idCompletionColumns.end();
         ++current)
    {
        mDelegateFactories->add(*current, new CSVWorld::IdCompletionDelegateFactory());
    }

    struct Mapping
    {
        CSMWorld::ColumnBase::Display mDisplay;
        CSMWorld::Columns::ColumnId mColumnId;
        bool mAllowNone;
    };

    static const Mapping sMapping[] =
    {
        { CSMWorld::ColumnBase::Display_Specialisation, CSMWorld::Columns::ColumnId_Specialisation, false },
        { CSMWorld::ColumnBase::Display_Attribute, CSMWorld::Columns::ColumnId_Attribute, true },
        { CSMWorld::ColumnBase::Display_SpellType, CSMWorld::Columns::ColumnId_SpellType, false },
        { CSMWorld::ColumnBase::Display_ApparatusType, CSMWorld::Columns::ColumnId_ApparatusType, false },
        { CSMWorld::ColumnBase::Display_ArmorType, CSMWorld::Columns::ColumnId_ArmorType, false },
        { CSMWorld::ColumnBase::Display_ClothingType, CSMWorld::Columns::ColumnId_ClothingType, false },
        { CSMWorld::ColumnBase::Display_CreatureType, CSMWorld::Columns::ColumnId_CreatureType, false },
        { CSMWorld::ColumnBase::Display_WeaponType, CSMWorld::Columns::ColumnId_WeaponType, false },
        { CSMWorld::ColumnBase::Display_DialogueType, CSMWorld::Columns::ColumnId_DialogueType, false },
        { CSMWorld::ColumnBase::Display_QuestStatusType, CSMWorld::Columns::ColumnId_QuestStatusType, false },
        { CSMWorld::ColumnBase::Display_EnchantmentType, CSMWorld::Columns::ColumnId_EnchantmentType, false },
        { CSMWorld::ColumnBase::Display_BodyPartType, CSMWorld::Columns::ColumnId_BodyPartType, false },
        { CSMWorld::ColumnBase::Display_MeshType, CSMWorld::Columns::ColumnId_MeshType, false },
        { CSMWorld::ColumnBase::Display_Gender, CSMWorld::Columns::ColumnId_Gender, true },
        { CSMWorld::ColumnBase::Display_SoundGeneratorType, CSMWorld::Columns::ColumnId_SoundGeneratorType, false },
        { CSMWorld::ColumnBase::Display_School, CSMWorld::Columns::ColumnId_School, false },
        { CSMWorld::ColumnBase::Display_SkillId, CSMWorld::Columns::ColumnId_Skill, true },
        { CSMWorld::ColumnBase::Display_EffectRange, CSMWorld::Columns::ColumnId_EffectRange, false },
        { CSMWorld::ColumnBase::Display_EffectId, CSMWorld::Columns::ColumnId_EffectId, false },
        { CSMWorld::ColumnBase::Display_PartRefType, CSMWorld::Columns::ColumnId_PartRefType, false },
        { CSMWorld::ColumnBase::Display_AiPackageType, CSMWorld::Columns::ColumnId_AiPackageType, false },
        { CSMWorld::ColumnBase::Display_InfoCondFunc, CSMWorld::Columns::ColumnId_InfoCondFunc, false },
        { CSMWorld::ColumnBase::Display_InfoCondComp, CSMWorld::Columns::ColumnId_InfoCondComp, false },
        { CSMWorld::ColumnBase::Display_IngredEffectId, CSMWorld::Columns::ColumnId_EffectId, true },
        { CSMWorld::ColumnBase::Display_EffectSkill, CSMWorld::Columns::ColumnId_Skill, false },
        { CSMWorld::ColumnBase::Display_EffectAttribute, CSMWorld::Columns::ColumnId_Attribute, false },
        { CSMWorld::ColumnBase::Display_BookType, CSMWorld::Columns::ColumnId_BookType, false },
        { CSMWorld::ColumnBase::Display_BloodType, CSMWorld::Columns::ColumnId_BloodType, false },
        { CSMWorld::ColumnBase::Display_EmitterType, CSMWorld::Columns::ColumnId_EmitterType, false },
        { CSMWorld::ColumnBase::Display_GenderNpc, CSMWorld::Columns::ColumnId_Gender, false }
    };

    for (std::size_t i=0; i<sizeof (sMapping)/sizeof (Mapping); ++i)
        mDelegateFactories->add (sMapping[i].mDisplay, new CSVWorld::EnumDelegateFactory (
            CSMWorld::Columns::getEnums (sMapping[i].mColumnId), sMapping[i].mAllowNone));

    connect (&mDocumentManager, SIGNAL (loadRequest (CSMDoc::Document *)),
        &mLoader, SLOT (add (CSMDoc::Document *)));

    connect (
        &mDocumentManager, SIGNAL (loadingStopped (CSMDoc::Document *, bool, const std::string&)),
        &mLoader, SLOT (loadingStopped (CSMDoc::Document *, bool, const std::string&)));

    connect (
        &mDocumentManager, SIGNAL (nextStage (CSMDoc::Document *, const std::string&, int)),
        &mLoader, SLOT (nextStage (CSMDoc::Document *, const std::string&, int)));

    connect (
        &mDocumentManager, SIGNAL (nextRecord (CSMDoc::Document *, int)),
        &mLoader, SLOT (nextRecord (CSMDoc::Document *, int)));

    connect (
        &mDocumentManager, SIGNAL (loadMessage (CSMDoc::Document *, const std::string&)),
        &mLoader, SLOT (loadMessage (CSMDoc::Document *, const std::string&)));

    connect (
        &mLoader, SIGNAL (cancel (CSMDoc::Document *)),
        &mDocumentManager, SIGNAL (cancelLoading (CSMDoc::Document *)));

    connect (
        &mLoader, SIGNAL (close (CSMDoc::Document *)),
        &mDocumentManager, SLOT (removeDocument (CSMDoc::Document *)));
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

    view->toggleStatusBar (CSMPrefs::get()["Windows"]["show-statusbar"].isTrue());
    view->show();

    connect (view, SIGNAL (newGameRequest ()), this, SIGNAL (newGameRequest()));
    connect (view, SIGNAL (newAddonRequest ()), this, SIGNAL (newAddonRequest()));
    connect (view, SIGNAL (loadDocumentRequest ()), this, SIGNAL (loadDocumentRequest()));
    connect (view, SIGNAL (editSettingsRequest()), this, SIGNAL (editSettingsRequest()));
    connect (view, SIGNAL (mergeDocument (CSMDoc::Document *)), this, SIGNAL (mergeDocument (CSMDoc::Document *)));

    updateIndices();

    return view;
}

CSVDoc::View *CSVDoc::ViewManager::addView (CSMDoc::Document *document, const CSMWorld::UniversalId& id, const std::string& hint)
{
    View* view = addView(document);
    view->addSubView(id, hint);

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

    bool continueWithClose = false;

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

// NOTE: This method assumes that it is called only if the last document
void CSVDoc::ViewManager::removeDocAndView (CSMDoc::Document *document)
{
    for (std::vector<View *>::iterator iter (mViews.begin()); iter!=mViews.end(); ++iter)
    {
        // the first match should also be the only match
        if((*iter)->getDocument() == document)
        {
            mDocumentManager.removeDocument(document);
            (*iter)->deleteLater();
            mViews.erase (iter);

            updateIndices();
            return;
        }
    }
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
    emit closeMessageBox();

    QMessageBox messageBox(view);
    CSMDoc::Document *document = view->getDocument();

    messageBox.setWindowTitle (QString::fromUtf8(document->getSavePath().filename().string().c_str()));
    messageBox.setText ("The document has been modified.");
    messageBox.setInformativeText ("Do you want to save your changes?");
    messageBox.setStandardButtons (QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    messageBox.setDefaultButton (QMessageBox::Save);
    messageBox.setWindowModality (Qt::NonModal);
    messageBox.hide();
    messageBox.show();

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

bool CSVDoc::ViewManager::removeDocument (CSVDoc::View *view)
{
    if(!notifySaveOnClose(view))
        return false;
    else
    {
        // don't bother closing views or updating indicies, but remove from mViews
        CSMDoc::Document * document = view->getDocument();
        std::vector<View *> remainingViews;
        std::vector<View *>::const_iterator iter = mViews.begin();
        for (; iter!=mViews.end(); ++iter)
        {
            if(document == (*iter)->getDocument())
                (*iter)->setVisible(false);
            else
                remainingViews.push_back(*iter);
        }
        mDocumentManager.removeDocument(document);
        mViews = remainingViews;
    }
    return true;
}

void CSVDoc::ViewManager::exitApplication (CSVDoc::View *view)
{
    if(!removeDocument(view)) // close the current document first
        return;

    while(!mViews.empty()) // attempt to close all other documents
    {
        mViews.back()->activateWindow();
        mViews.back()->raise(); // raise the window to alert the user
        if(!removeDocument(mViews.back()))
            return;
    }
    // Editor exits (via a signal) when the last document is deleted
}
