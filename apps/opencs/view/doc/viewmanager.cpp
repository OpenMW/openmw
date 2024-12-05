#include "viewmanager.hpp"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <map>
#include <type_traits>
#include <utility>
#include <vector>

#include <apps/opencs/model/prefs/category.hpp>
#include <apps/opencs/model/prefs/setting.hpp>
#include <apps/opencs/model/world/columnbase.hpp>
#include <apps/opencs/model/world/universalid.hpp>
#include <apps/opencs/view/doc/loader.hpp>

#include <components/esm3/variant.hpp>
#include <components/files/qtconversion.hpp>

#include <QApplication>
#include <QMessageBox>
#include <QPushButton>

#include "../../model/doc/document.hpp"
#include "../../model/doc/documentmanager.hpp"

#include "../../model/prefs/state.hpp"

#include "../world/colordelegate.hpp"
#include "../world/enumdelegate.hpp"
#include "../world/idcompletiondelegate.hpp"
#include "../world/idtypedelegate.hpp"
#include "../world/recordstatusdelegate.hpp"
#include "../world/vartypedelegate.hpp"

#include "view.hpp"

void CSVDoc::ViewManager::updateIndices()
{
    std::map<CSMDoc::Document*, std::pair<int, int>> documents;

    for (std::vector<View*>::const_iterator iter(mViews.begin()); iter != mViews.end(); ++iter)
    {
        std::map<CSMDoc::Document*, std::pair<int, int>>::iterator document = documents.find((*iter)->getDocument());

        if (document == documents.end())
            document = documents
                           .insert(std::make_pair(
                               (*iter)->getDocument(), std::make_pair(0, countViews((*iter)->getDocument()))))
                           .first;

        (*iter)->setIndex(document->second.first++, document->second.second);
    }
}

CSVDoc::ViewManager::ViewManager(CSMDoc::DocumentManager& documentManager)
    : mDocumentManager(documentManager)
    , mExitOnSaveStateChange(false)
    , mUserWarned(false)
{
    mDelegateFactories = new CSVWorld::CommandDelegateFactoryCollection;

    mDelegateFactories->add(CSMWorld::ColumnBase::Display_GmstVarType,
        new CSVWorld::VarTypeDelegateFactory(ESM::VT_None, ESM::VT_String, ESM::VT_Int, ESM::VT_Float));

    mDelegateFactories->add(CSMWorld::ColumnBase::Display_GlobalVarType,
        new CSVWorld::VarTypeDelegateFactory(ESM::VT_Short, ESM::VT_Long, ESM::VT_Float));

    mDelegateFactories->add(CSMWorld::ColumnBase::Display_RecordState, new CSVWorld::RecordStatusDelegateFactory());

    mDelegateFactories->add(CSMWorld::ColumnBase::Display_RefRecordType, new CSVWorld::IdTypeDelegateFactory());

    mDelegateFactories->add(CSMWorld::ColumnBase::Display_Colour, new CSVWorld::ColorDelegateFactory());

    std::vector<CSMWorld::ColumnBase::Display> idCompletionColumns = CSMWorld::IdCompletionManager::getDisplayTypes();
    for (std::vector<CSMWorld::ColumnBase::Display>::const_iterator current = idCompletionColumns.begin();
         current != idCompletionColumns.end(); ++current)
    {
        mDelegateFactories->add(*current, new CSVWorld::IdCompletionDelegateFactory());
    }

    struct Mapping
    {
        CSMWorld::ColumnBase::Display mDisplay;
        CSMWorld::Columns::ColumnId mColumnId;
        bool mAllowNone;
    };

    static const Mapping sMapping[] = {
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
        { CSMWorld::ColumnBase::Display_GenderNpc, CSMWorld::Columns::ColumnId_Gender, false },
    };

    for (std::size_t i = 0; i < sizeof(sMapping) / sizeof(Mapping); ++i)
        mDelegateFactories->add(sMapping[i].mDisplay,
            new CSVWorld::EnumDelegateFactory(
                CSMWorld::Columns::getEnums(sMapping[i].mColumnId), sMapping[i].mAllowNone));

    connect(&mDocumentManager, &CSMDoc::DocumentManager::loadRequest, &mLoader, &Loader::add);

    connect(&mDocumentManager, &CSMDoc::DocumentManager::loadingStopped, &mLoader, &Loader::loadingStopped);

    connect(&mDocumentManager, &CSMDoc::DocumentManager::nextStage, &mLoader, &Loader::nextStage);

    connect(&mDocumentManager, &CSMDoc::DocumentManager::nextRecord, &mLoader, &Loader::nextRecord);

    connect(&mDocumentManager, &CSMDoc::DocumentManager::loadMessage, &mLoader, &Loader::loadMessage);

    connect(&mLoader, &Loader::cancel, &mDocumentManager, &CSMDoc::DocumentManager::cancelLoading);

    connect(&mLoader, &Loader::close, &mDocumentManager, &CSMDoc::DocumentManager::removeDocument);
}

CSVDoc::ViewManager::~ViewManager()
{
    delete mDelegateFactories;

    for (std::vector<View*>::iterator iter(mViews.begin()); iter != mViews.end(); ++iter)
        delete *iter;
}

CSVDoc::View* CSVDoc::ViewManager::addView(CSMDoc::Document* document)
{
    if (countViews(document) == 0)
    {
        // new document
        connect(document, &CSMDoc::Document::stateChanged, this, &ViewManager::documentStateChanged);

        connect(document, qOverload<int, int, int, int, CSMDoc::Document*>(&CSMDoc::Document::progress), this,
            &ViewManager::progress);
    }

    View* view = new View(*this, document, countViews(document) + 1);

    mViews.push_back(view);

    view->toggleStatusBar(CSMPrefs::get()["Windows"]["show-statusbar"].isTrue());
    view->show();

    connect(view, &View::newGameRequest, this, &ViewManager::newGameRequest);
    connect(view, &View::newAddonRequest, this, &ViewManager::newAddonRequest);
    connect(view, &View::loadDocumentRequest, this, &ViewManager::loadDocumentRequest);
    connect(view, &View::editSettingsRequest, this, &ViewManager::editSettingsRequest);
    connect(view, &View::mergeDocument, this, &ViewManager::mergeDocument);

    updateIndices();

    return view;
}

CSVDoc::View* CSVDoc::ViewManager::addView(
    CSMDoc::Document* document, const CSMWorld::UniversalId& id, const std::string& hint)
{
    View* view = addView(document);
    view->addSubView(id, hint);

    return view;
}

int CSVDoc::ViewManager::countViews(const CSMDoc::Document* document) const
{
    int count = 0;

    for (std::vector<View*>::const_iterator iter(mViews.begin()); iter != mViews.end(); ++iter)
        if ((*iter)->getDocument() == document)
            ++count;

    return count;
}

bool CSVDoc::ViewManager::closeRequest(View* view)
{
    std::vector<View*>::iterator iter = std::find(mViews.begin(), mViews.end(), view);

    bool continueWithClose = false;

    if (iter != mViews.end())
    {
        bool last = countViews(view->getDocument()) <= 1;

        if (last)
            continueWithClose = notifySaveOnClose(view);
        else
        {
            (*iter)->deleteLater();
            mViews.erase(iter);

            updateIndices();
        }
    }

    return continueWithClose;
}

// NOTE: This method assumes that it is called only if the last document
void CSVDoc::ViewManager::removeDocAndView(CSMDoc::Document* document)
{
    for (std::vector<View*>::iterator iter(mViews.begin()); iter != mViews.end(); ++iter)
    {
        // the first match should also be the only match
        if ((*iter)->getDocument() == document)
        {
            mDocumentManager.removeDocument(document);
            (*iter)->deleteLater();
            mViews.erase(iter);

            updateIndices();
            return;
        }
    }
}

bool CSVDoc::ViewManager::notifySaveOnClose(CSVDoc::View* view)
{
    bool result = true;
    CSMDoc::Document* document = view->getDocument();

    // notify user of saving in progress
    if ((document->getState() & CSMDoc::State_Saving))
        result = showSaveInProgressMessageBox(view);

    // notify user of unsaved changes and process response
    else if (document->getState() & CSMDoc::State_Modified)
        result = showModifiedDocumentMessageBox(view);

    return result;
}

bool CSVDoc::ViewManager::showModifiedDocumentMessageBox(CSVDoc::View* view)
{
    emit closeMessageBox();

    QMessageBox messageBox(view);
    CSMDoc::Document* document = view->getDocument();

    messageBox.setWindowTitle(Files::pathToQString(document->getSavePath().filename()));
    messageBox.setText("The document has been modified.");
    messageBox.setInformativeText("Do you want to save your changes?");
    messageBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    messageBox.setDefaultButton(QMessageBox::Save);
    messageBox.setWindowModality(Qt::NonModal);
    messageBox.hide();
    messageBox.show();

    bool retVal = true;

    connect(this, &ViewManager::closeMessageBox, &messageBox, &QMessageBox::close);

    connect(document, &CSMDoc::Document::stateChanged, this, &ViewManager::onExitWarningHandler);

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

            disconnect(document, &CSMDoc::Document::stateChanged, this, &ViewManager::onExitWarningHandler);
            break;

        case QMessageBox::Cancel:

            // disconnect to prevent unintended view closures
            disconnect(document, &CSMDoc::Document::stateChanged, this, &ViewManager::onExitWarningHandler);
            retVal = false;
            break;

        default:
            break;
    }

    return retVal;
}

bool CSVDoc::ViewManager::showSaveInProgressMessageBox(CSVDoc::View* view)
{
    QMessageBox messageBox;
    CSMDoc::Document* document = view->getDocument();

    messageBox.setText("The document is currently being saved.");
    messageBox.setInformativeText("Do you want to close now and abort saving, or wait until saving has completed?");

    QPushButton* waitButton = messageBox.addButton(tr("Wait"), QMessageBox::YesRole);
    QPushButton* closeButton = messageBox.addButton(tr("Close Now"), QMessageBox::RejectRole);
    QPushButton* cancelButton = messageBox.addButton(tr("Cancel"), QMessageBox::NoRole);

    messageBox.setDefaultButton(waitButton);

    bool retVal = true;

    // Connections shut down message box if operation ends before user makes a decision.
    connect(document, &CSMDoc::Document::stateChanged, this, &ViewManager::onExitWarningHandler);
    connect(this, &ViewManager::closeMessageBox, &messageBox, &QMessageBox::close);

    // set / clear the user warned flag to indicate whether or not the message box is currently active.
    mUserWarned = true;
    messageBox.exec();
    mUserWarned = false;

    // if closed by the warning handler, defaults to the RejectRole button (closeButton)
    if (messageBox.clickedButton() == waitButton)
    {
        // save the View iterator for shutdown after the save operation ends
        mExitOnSaveStateChange = true;
        retVal = false;
    }

    else if (messageBox.clickedButton() == closeButton)
    {
        // disconnect to avoid segmentation fault
        disconnect(document, &CSMDoc::Document::stateChanged, this, &ViewManager::onExitWarningHandler);

        view->abortOperation(CSMDoc::State_Saving);
        mExitOnSaveStateChange = true;
    }

    else if (messageBox.clickedButton() == cancelButton)
    {
        // abort shutdown, allow save to complete
        // disconnection to prevent unintended view closures
        mExitOnSaveStateChange = false;
        disconnect(document, &CSMDoc::Document::stateChanged, this, &ViewManager::onExitWarningHandler);
        retVal = false;
    }

    return retVal;
}

void CSVDoc::ViewManager::documentStateChanged(int state, CSMDoc::Document* document)
{
    for (std::vector<View*>::const_iterator iter(mViews.begin()); iter != mViews.end(); ++iter)
        if ((*iter)->getDocument() == document)
            (*iter)->updateDocumentState();
}

void CSVDoc::ViewManager::progress(int current, int max, int type, int threads, CSMDoc::Document* document)
{
    for (std::vector<View*>::const_iterator iter(mViews.begin()); iter != mViews.end(); ++iter)
        if ((*iter)->getDocument() == document)
            (*iter)->updateProgress(current, max, type, threads);
}

void CSVDoc::ViewManager::onExitWarningHandler(int state, CSMDoc::Document* document)
{
    if (!(state & CSMDoc::State_Saving))
    {
        // if the user is being warned (message box is active), shut down the message box,
        // as there is no save operation currently running
        if (mUserWarned)
            emit closeMessageBox();

        // otherwise, the user has closed the message box before the save operation ended.
        // exit the application
        else if (mExitOnSaveStateChange)
            QApplication::instance()->exit();
    }
}

bool CSVDoc::ViewManager::removeDocument(CSVDoc::View* view)
{
    if (!notifySaveOnClose(view))
        return false;
    else
    {
        // don't bother closing views or updating indicies, but remove from mViews
        CSMDoc::Document* document = view->getDocument();
        std::vector<View*> remainingViews;
        std::vector<View*>::const_iterator iter = mViews.begin();
        for (; iter != mViews.end(); ++iter)
        {
            if (document == (*iter)->getDocument())
                (*iter)->setVisible(false);
            else
                remainingViews.push_back(*iter);
        }
        mDocumentManager.removeDocument(document);
        mViews = std::move(remainingViews);
    }
    return true;
}

void CSVDoc::ViewManager::exitApplication(CSVDoc::View* view)
{
    if (!removeDocument(view)) // close the current document first
        return;

    while (!mViews.empty()) // attempt to close all other documents
    {
        mViews.back()->activateWindow();
        mViews.back()->raise(); // raise the window to alert the user
        if (!removeDocument(mViews.back()))
            return;
    }
    // Editor exits (via a signal) when the last document is deleted
}
