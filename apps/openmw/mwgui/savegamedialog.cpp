#include "savegamedialog.hpp"

#include <sstream>
#include <iomanip>

#include <MyGUI_ComboBox.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_ListBox.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_LanguageManager.h>

#include <osgDB/ReadFile>
#include <osg/Texture2D>

#include <components/debug/debuglog.hpp>

#include <components/myguiplatform/myguitexture.hpp>

#include <components/misc/stringops.hpp>

#include <components/settings/settings.hpp>

#include <components/files/memorystream.hpp>

#include "../mwbase/statemanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwstate/character.hpp"

#include "confirmationdialog.hpp"

namespace MWGui
{
    SaveGameDialog::SaveGameDialog()
        : WindowModal("openmw_savegame_dialog.layout")
        , mSaving(true)
        , mCurrentCharacter(nullptr)
        , mCurrentSlot(nullptr)
    {
        getWidget(mScreenshot, "Screenshot");
        getWidget(mCharacterSelection, "SelectCharacter");
        getWidget(mInfoText, "InfoText");
        getWidget(mOkButton, "OkButton");
        getWidget(mCancelButton, "CancelButton");
        getWidget(mDeleteButton, "DeleteButton");
        getWidget(mSaveList, "SaveList");
        getWidget(mSaveNameEdit, "SaveNameEdit");
        getWidget(mSpacer, "Spacer");
        mOkButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SaveGameDialog::onOkButtonClicked);
        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SaveGameDialog::onCancelButtonClicked);
        mDeleteButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SaveGameDialog::onDeleteButtonClicked);
        mCharacterSelection->eventComboChangePosition += MyGUI::newDelegate(this, &SaveGameDialog::onCharacterSelected);
        mCharacterSelection->eventComboAccept += MyGUI::newDelegate(this, &SaveGameDialog::onCharacterAccept);
        mSaveList->eventListChangePosition += MyGUI::newDelegate(this, &SaveGameDialog::onSlotSelected);
        mSaveList->eventListMouseItemActivate += MyGUI::newDelegate(this, &SaveGameDialog::onSlotMouseClick);
        mSaveList->eventListSelectAccept += MyGUI::newDelegate(this, &SaveGameDialog::onSlotActivated);
        mSaveList->eventKeyButtonPressed += MyGUI::newDelegate(this, &SaveGameDialog::onKeyButtonPressed);
        mSaveNameEdit->eventEditSelectAccept += MyGUI::newDelegate(this, &SaveGameDialog::onEditSelectAccept);
        mSaveNameEdit->eventEditTextChange += MyGUI::newDelegate(this, &SaveGameDialog::onSaveNameChanged);

        // To avoid accidental deletions
        mDeleteButton->setNeedKeyFocus(false);
    }

    void SaveGameDialog::onSlotActivated(MyGUI::ListBox *sender, size_t pos)
    {
        onSlotSelected(sender, pos);
        accept();
    }

    void SaveGameDialog::onSlotMouseClick(MyGUI::ListBox* sender, size_t pos)
    {
        onSlotSelected(sender, pos);

        if (pos != MyGUI::ITEM_NONE && MyGUI::InputManager::getInstance().isShiftPressed())
            confirmDeleteSave();
    }

    void SaveGameDialog::confirmDeleteSave()
    {
        ConfirmationDialog* dialog = MWBase::Environment::get().getWindowManager()->getConfirmationDialog();
        dialog->askForConfirmation("#{sMessage3}");
        dialog->eventOkClicked.clear();
        dialog->eventOkClicked += MyGUI::newDelegate(this, &SaveGameDialog::onDeleteSlotConfirmed);
        dialog->eventCancelClicked.clear();
        dialog->eventCancelClicked += MyGUI::newDelegate(this, &SaveGameDialog::onDeleteSlotCancel);
    }

    void SaveGameDialog::onDeleteSlotConfirmed()
    {
        MWBase::Environment::get().getStateManager()->deleteGame (mCurrentCharacter, mCurrentSlot);
        mSaveList->removeItemAt(mSaveList->getIndexSelected());
        onSlotSelected(mSaveList, mSaveList->getIndexSelected());
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mSaveList);

        if (mSaveList->getItemCount() == 0)
        {
            size_t previousIndex = mCharacterSelection->getIndexSelected();
            mCurrentCharacter = nullptr;
            mCharacterSelection->removeItemAt(previousIndex);
            if (mCharacterSelection->getItemCount())
            {
                size_t nextCharacter = std::min(previousIndex, mCharacterSelection->getItemCount()-1);
                mCharacterSelection->setIndexSelected(nextCharacter);
                onCharacterSelected(mCharacterSelection, nextCharacter);
            }
            else
                fillSaveList();
        }
    }

    void SaveGameDialog::onDeleteSlotCancel()
    {
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mSaveList);
    }

    void SaveGameDialog::onSaveNameChanged(MyGUI::EditBox *sender)
    {
        // This might have previously been a save slot from the list. If so, that is no longer the case
        mSaveList->setIndexSelected(MyGUI::ITEM_NONE);
        onSlotSelected(mSaveList, MyGUI::ITEM_NONE);
    }

    void SaveGameDialog::onEditSelectAccept(MyGUI::EditBox *sender)
    {
        accept();

        // To do not spam onEditSelectAccept() again and again
        MWBase::Environment::get().getWindowManager()->injectKeyRelease(MyGUI::KeyCode::None);
    }

    void SaveGameDialog::onOpen()
    {
        WindowModal::onOpen();

        mSaveNameEdit->setCaption ("");
        if (mSaving)
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mSaveNameEdit);
        else
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mSaveList);

        center();

        mCharacterSelection->setCaption("");
        mCharacterSelection->removeAllItems();
        mCurrentCharacter = nullptr;
        mCurrentSlot = nullptr;
        mSaveList->removeAllItems();
        onSlotSelected(mSaveList, MyGUI::ITEM_NONE);

        MWBase::StateManager* mgr = MWBase::Environment::get().getStateManager();
        if (mgr->characterBegin() == mgr->characterEnd())
            return;

        mCurrentCharacter = mgr->getCurrentCharacter();

        std::string directory =
            Misc::StringUtils::lowerCase (Settings::Manager::getString ("character", "Saves"));

        size_t selectedIndex = MyGUI::ITEM_NONE;

        for (MWBase::StateManager::CharacterIterator it = mgr->characterBegin(); it != mgr->characterEnd(); ++it)
        {
            if (it->begin()!=it->end())
            {
                std::stringstream title;
                title << it->getSignature().mPlayerName;

                // For a custom class, we will not find it in the store (unless we loaded the savegame first).
                // Fall back to name stored in savegame header in that case.
                std::string className;
                if (it->getSignature().mPlayerClassId.empty())
                    className = it->getSignature().mPlayerClassName;
                else
                {
                    // Find the localised name for this class from the store
                    const ESM::Class* class_ = MWBase::Environment::get().getWorld()->getStore().get<ESM::Class>().search(
                                it->getSignature().mPlayerClassId);
                    if (class_)
                        className = class_->mName;
                    else
                        className = "?"; // From an older savegame format that did not support custom classes properly.
                }

                title << " (#{sLevel} " << it->getSignature().mPlayerLevel << " " << MyGUI::TextIterator::toTagsString(className) << ")";

                mCharacterSelection->addItem (MyGUI::LanguageManager::getInstance().replaceTags(title.str()));

                if (mCurrentCharacter == &*it ||
                    (!mCurrentCharacter && !mSaving && directory==Misc::StringUtils::lowerCase (
                    it->begin()->mPath.parent_path().filename().string())))
                {
                    mCurrentCharacter = &*it;
                    selectedIndex = mCharacterSelection->getItemCount()-1;
                }
            }
        }

        mCharacterSelection->setIndexSelected(selectedIndex);
        if (selectedIndex == MyGUI::ITEM_NONE)
            mCharacterSelection->setCaption("Select Character ...");

        fillSaveList();

    }

    void SaveGameDialog::setLoadOrSave(bool load)
    {
        mSaving = !load;
        mSaveNameEdit->setVisible(!load);
        mCharacterSelection->setUserString("Hidden", load ? "false" : "true");
        mCharacterSelection->setVisible(load);
        mSpacer->setUserString("Hidden", load ? "false" : "true");

        mDeleteButton->setUserString("Hidden", load ? "false" : "true");
        mDeleteButton->setVisible(load);

        if (!load)
        {
            mCurrentCharacter = MWBase::Environment::get().getStateManager()->getCurrentCharacter();
        }

        center();
    }

    void SaveGameDialog::onCancelButtonClicked(MyGUI::Widget *sender)
    {
        setVisible(false);
    }

    void SaveGameDialog::onDeleteButtonClicked(MyGUI::Widget *sender)
    {
        if (mCurrentSlot)
            confirmDeleteSave();
    }

    void SaveGameDialog::onConfirmationGiven()
    {
        accept(true);
    }

    void SaveGameDialog::onConfirmationCancel()
    {
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mSaveList);
    }

    void SaveGameDialog::accept(bool reallySure)
    {
        if (mSaving)
        {
            // If overwriting an existing slot, ask for confirmation first
            if (mCurrentSlot != nullptr && !reallySure)
            {
                ConfirmationDialog* dialog = MWBase::Environment::get().getWindowManager()->getConfirmationDialog();
                dialog->askForConfirmation("#{sMessage4}");
                dialog->eventOkClicked.clear();
                dialog->eventOkClicked += MyGUI::newDelegate(this, &SaveGameDialog::onConfirmationGiven);
                dialog->eventCancelClicked.clear();
                dialog->eventCancelClicked += MyGUI::newDelegate(this, &SaveGameDialog::onConfirmationCancel);
                return;
            }
            if (mSaveNameEdit->getCaption().empty())
            {
                MWBase::Environment::get().getWindowManager()->messageBox("#{sNotifyMessage65}");
                return;
            }
        }
        else
        {
            MWBase::StateManager::State state = MWBase::Environment::get().getStateManager()->getState();

            // If game is running, ask for confirmation first
            if (state == MWBase::StateManager::State_Running && !reallySure)
            {
                ConfirmationDialog* dialog = MWBase::Environment::get().getWindowManager()->getConfirmationDialog();
                dialog->askForConfirmation("#{sMessage1}");
                dialog->eventOkClicked.clear();
                dialog->eventOkClicked += MyGUI::newDelegate(this, &SaveGameDialog::onConfirmationGiven);
                dialog->eventCancelClicked.clear();
                dialog->eventCancelClicked += MyGUI::newDelegate(this, &SaveGameDialog::onConfirmationCancel);
                return;
            }
        }

        setVisible(false);
        MWBase::Environment::get().getWindowManager()->removeGuiMode (MWGui::GM_MainMenu);

        if (mSaving)
        {
            MWBase::Environment::get().getStateManager()->saveGame (mSaveNameEdit->getCaption(), mCurrentSlot);
        }
        else
        {
            assert (mCurrentCharacter && mCurrentSlot);
            MWBase::Environment::get().getStateManager()->loadGame (mCurrentCharacter, mCurrentSlot->mPath.string());
        }
    }

    void SaveGameDialog::onKeyButtonPressed(MyGUI::Widget* _sender, MyGUI::KeyCode key, MyGUI::Char character)
    {
        if (key == MyGUI::KeyCode::Delete && mCurrentSlot)
            confirmDeleteSave();
    }

    void SaveGameDialog::onOkButtonClicked(MyGUI::Widget *sender)
    {
        accept();
    }

    void SaveGameDialog::onCharacterSelected(MyGUI::ComboBox *sender, size_t pos)
    {
        MWBase::StateManager* mgr = MWBase::Environment::get().getStateManager();

        unsigned int i=0;
        const MWState::Character* character = nullptr;
        for (MWBase::StateManager::CharacterIterator it = mgr->characterBegin(); it != mgr->characterEnd(); ++it, ++i)
        {
            if (i == pos)
                character = &*it;
        }
        assert(character && "Can't find selected character");

        mCurrentCharacter = character;
        mCurrentSlot = nullptr;
        fillSaveList();
    }

    void SaveGameDialog::onCharacterAccept(MyGUI::ComboBox* sender, size_t pos)
    {
        // Give key focus to save list so we can confirm the selection with Enter
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mSaveList);
    }

    void SaveGameDialog::fillSaveList()
    {
        mSaveList->removeAllItems();
        if (!mCurrentCharacter)
            return;
        for (MWState::Character::SlotIterator it = mCurrentCharacter->begin(); it != mCurrentCharacter->end(); ++it)
        {
            mSaveList->addItem(it->mProfile.mDescription);
        }
        // When loading, Auto-select the first save, if there is one
        if (mSaveList->getItemCount() && !mSaving)
        {
            mSaveList->setIndexSelected(0);
            onSlotSelected(mSaveList, 0);
        }
        else
            onSlotSelected(mSaveList, MyGUI::ITEM_NONE);
    }

    std::string formatTimeplayed(const double timeInSeconds)
    {
        int timePlayed = (int)floor(timeInSeconds);
        int days = timePlayed / 60 / 60 / 24;
        int hours = (timePlayed / 60 / 60) % 24;
        int minutes = (timePlayed / 60) % 60;
        int seconds = timePlayed % 60;

        std::stringstream stream;
        stream << std::setfill('0') << std::setw(2) << days << ":";
        stream << std::setfill('0') << std::setw(2) << hours << ":";
        stream << std::setfill('0') << std::setw(2) << minutes << ":";
        stream << std::setfill('0') << std::setw(2) << seconds;
        return stream.str();
    }

    void SaveGameDialog::onSlotSelected(MyGUI::ListBox *sender, size_t pos)
    {
        mOkButton->setEnabled(pos != MyGUI::ITEM_NONE || mSaving);
        mDeleteButton->setEnabled(pos != MyGUI::ITEM_NONE);

        if (pos == MyGUI::ITEM_NONE || !mCurrentCharacter)
        {
            mCurrentSlot = nullptr;
            mInfoText->setCaption("");
            mScreenshot->setImageTexture("");
            return;
        }

        if (mSaving)
            mSaveNameEdit->setCaption(sender->getItemNameAt(pos));

        mCurrentSlot = nullptr;
        unsigned int i=0;
        for (MWState::Character::SlotIterator it = mCurrentCharacter->begin(); it != mCurrentCharacter->end(); ++it, ++i)
        {
            if (i == pos)
                mCurrentSlot = &*it;
        }
        if (!mCurrentSlot)
            throw std::runtime_error("Can't find selected slot");

        std::stringstream text;
        time_t time = mCurrentSlot->mTimeStamp;
        struct tm* timeinfo;
        timeinfo = localtime(&time);

        text << std::put_time(timeinfo, "%Y.%m.%d %T") << "\n";

        text << "#{sLevel} " << mCurrentSlot->mProfile.mPlayerLevel << "\n";
        text << "#{sCell=" << mCurrentSlot->mProfile.mPlayerCell << "}\n";

        int hour = int(mCurrentSlot->mProfile.mInGameTime.mGameHour);
        bool pm = hour >= 12;
        if (hour >= 13) hour -= 12;
        if (hour == 0) hour = 12;

        text
            << mCurrentSlot->mProfile.mInGameTime.mDay << " "
            << MWBase::Environment::get().getWorld()->getMonthName(mCurrentSlot->mProfile.mInGameTime.mMonth)
            <<  " " << hour << " " << (pm ? "#{sSaveMenuHelp05}" : "#{sSaveMenuHelp04}");

        if (Settings::Manager::getBool("timeplayed","Saves"))
        {
            text << "\n" << "Time played: " << formatTimeplayed(mCurrentSlot->mProfile.mTimePlayed);
        }

        mInfoText->setCaptionWithReplacing(text.str());


        // Decode screenshot
        const std::vector<char>& data = mCurrentSlot->mProfile.mScreenshot;
        Files::IMemStream instream (&data[0], data.size());

        osgDB::ReaderWriter* readerwriter = osgDB::Registry::instance()->getReaderWriterForExtension("jpg");
        if (!readerwriter)
        {
            Log(Debug::Error) << "Error: Can't open savegame screenshot, no jpg readerwriter found";
            return;
        }

        osgDB::ReaderWriter::ReadResult result = readerwriter->readImage(instream);
        if (!result.success())
        {
            Log(Debug::Error) << "Error: Failed to read savegame screenshot: " << result.message() << " code " << result.status();
            return;
        }

        osg::ref_ptr<osg::Texture2D> texture (new osg::Texture2D);
        texture->setImage(result.getImage());
        texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        texture->setResizeNonPowerOfTwoHint(false);
        texture->setUnRefImageDataAfterApply(true);

        mScreenshotTexture.reset(new osgMyGUI::OSGTexture(texture));

        mScreenshot->setRenderItemTexture(mScreenshotTexture.get());
        mScreenshot->getSubWidgetMain()->_setUVSet(MyGUI::FloatRect(0.f, 0.f, 1.f, 1.f));
    }
}
