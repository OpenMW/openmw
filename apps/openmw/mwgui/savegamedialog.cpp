#include "savegamedialog.hpp"
#include "widgets.hpp"

#include <OgreImage.h>
#include <OgreTextureManager.h>

#include <MyGUI_ComboBox.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_ListBox.h>
#include <MyGUI_InputManager.h>

#include <components/misc/stringops.hpp>

#include <components/settings/settings.hpp>

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
        , mCurrentCharacter(NULL)
        , mCurrentSlot(NULL)
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
        mCharacterSelection->eventComboAccept += MyGUI::newDelegate(this, &SaveGameDialog::onCharacterSelected);
        mSaveList->eventListChangePosition += MyGUI::newDelegate(this, &SaveGameDialog::onSlotSelected);
        mSaveList->eventListMouseItemActivate += MyGUI::newDelegate(this, &SaveGameDialog::onSlotMouseClick);
        mSaveList->eventListSelectAccept += MyGUI::newDelegate(this, &SaveGameDialog::onSlotActivated);
        mSaveList->eventKeyButtonPressed += MyGUI::newDelegate(this, &SaveGameDialog::onKeyButtonPressed);
        mSaveNameEdit->eventEditSelectAccept += MyGUI::newDelegate(this, &SaveGameDialog::onEditSelectAccept);
        mSaveNameEdit->eventEditTextChange += MyGUI::newDelegate(this, &SaveGameDialog::onSaveNameChanged);
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
        dialog->open("#{sMessage3}");
        dialog->eventOkClicked.clear();
        dialog->eventOkClicked += MyGUI::newDelegate(this, &SaveGameDialog::onDeleteSlotConfirmed);
        dialog->eventCancelClicked.clear();
    }

    void SaveGameDialog::onDeleteSlotConfirmed()
    {
        MWBase::Environment::get().getStateManager()->deleteGame (mCurrentCharacter, mCurrentSlot);
        mSaveList->removeItemAt(mSaveList->getIndexSelected());
        onSlotSelected(mSaveList, MyGUI::ITEM_NONE);

        // The character might be deleted now
        size_t previousIndex = mCharacterSelection->getIndexSelected();
        open();
        if (mCharacterSelection->getItemCount())
        {
            size_t nextCharacter = std::min(previousIndex, mCharacterSelection->getItemCount()-1);
            mCharacterSelection->setIndexSelected(nextCharacter);
            onCharacterSelected(mCharacterSelection, nextCharacter);
        }
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
    }

    void SaveGameDialog::open()
    {
        WindowModal::open();

        mSaveNameEdit->setCaption ("");
        if (mSaving)
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mSaveNameEdit);

        center();

        mCharacterSelection->setCaption("");
        mCharacterSelection->removeAllItems();
        mCurrentCharacter = NULL;
        mCurrentSlot = NULL;
        mSaveList->removeAllItems();
        onSlotSelected(mSaveList, MyGUI::ITEM_NONE);

        MWBase::StateManager* mgr = MWBase::Environment::get().getStateManager();
        if (mgr->characterBegin() == mgr->characterEnd())
            return;

        mCurrentCharacter = mgr->getCurrentCharacter (false);

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

                title << " (Level " << it->getSignature().mPlayerLevel << " " << className << ")";

                mCharacterSelection->addItem (title.str());

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

    void SaveGameDialog::exit()
    {
        setVisible(false);
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
            mCurrentCharacter = MWBase::Environment::get().getStateManager()->getCurrentCharacter (false);
        }

        center();
    }

    void SaveGameDialog::onCancelButtonClicked(MyGUI::Widget *sender)
    {
        exit();
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

    void SaveGameDialog::accept(bool reallySure)
    {
        // Remove for MyGUI 3.2.2
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(NULL);

        if (mSaving)
        {
            // If overwriting an existing slot, ask for confirmation first
            if (mCurrentSlot != NULL && !reallySure)
            {
                ConfirmationDialog* dialog = MWBase::Environment::get().getWindowManager()->getConfirmationDialog();
                dialog->open("#{sMessage4}");
                dialog->eventOkClicked.clear();
                dialog->eventOkClicked += MyGUI::newDelegate(this, &SaveGameDialog::onConfirmationGiven);
                dialog->eventCancelClicked.clear();
                return;
            }
            if (mSaveNameEdit->getCaption().empty())
            {
                MWBase::Environment::get().getWindowManager()->messageBox("#{sNotifyMessage65}");
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
        const MWState::Character* character = NULL;
        for (MWBase::StateManager::CharacterIterator it = mgr->characterBegin(); it != mgr->characterEnd(); ++it, ++i)
        {
            if (i == pos)
                character = &*it;
        }
        assert(character && "Can't find selected character");

        mCurrentCharacter = character;
        mCurrentSlot = NULL;
        fillSaveList();
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
            // Give key focus to save list so we can confirm the selection with Enter
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mSaveList);
        }
        else
            onSlotSelected(mSaveList, MyGUI::ITEM_NONE);
    }

    void SaveGameDialog::onSlotSelected(MyGUI::ListBox *sender, size_t pos)
    {
        mOkButton->setEnabled(pos != MyGUI::ITEM_NONE || mSaving);
        mDeleteButton->setEnabled(pos != MyGUI::ITEM_NONE);

        if (pos == MyGUI::ITEM_NONE || !mCurrentCharacter)
        {
            mCurrentSlot = NULL;
            mInfoText->setCaption("");
            mScreenshot->setImageTexture("");
            return;
        }

        if (mSaving)
            mSaveNameEdit->setCaption(sender->getItemNameAt(pos));

        mCurrentSlot = NULL;
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

        // Use system/environment locale settings for datetime formatting
        setlocale(LC_TIME, "");

        const int size=1024;
        char buffer[size];
        if (std::strftime(buffer, size, "%x %X", timeinfo) > 0)
            text << buffer << "\n";
        text << "Level " << mCurrentSlot->mProfile.mPlayerLevel << "\n";
        text << mCurrentSlot->mProfile.mPlayerCell << "\n";
        // text << "Time played: " << slot->mProfile.mTimePlayed << "\n";

        int hour = int(mCurrentSlot->mProfile.mInGameTime.mGameHour);
        bool pm = hour >= 12;
        if (hour >= 13) hour -= 12;
        if (hour == 0) hour = 12;

        text
            << mCurrentSlot->mProfile.mInGameTime.mDay << " "
            << MWBase::Environment::get().getWorld()->getMonthName(mCurrentSlot->mProfile.mInGameTime.mMonth)
            <<  " " << hour << " " << (pm ? "#{sSaveMenuHelp05}" : "#{sSaveMenuHelp04}");

        mInfoText->setCaptionWithReplacing(text.str());

        // Decode screenshot
        std::vector<char> data = mCurrentSlot->mProfile.mScreenshot; // MemoryDataStream doesn't work with const data :(
        Ogre::DataStreamPtr stream(new Ogre::MemoryDataStream(&data[0], data.size()));
        Ogre::Image image;
        image.load(stream, "jpg");

        const std::string textureName = "@savegame_screenshot";
        Ogre::TexturePtr texture;
        texture = Ogre::TextureManager::getSingleton().getByName(textureName);
        mScreenshot->setImageTexture("");
        if (texture.isNull())
        {
            texture = Ogre::TextureManager::getSingleton().createManual(textureName,
                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                Ogre::TEX_TYPE_2D,
                image.getWidth(), image.getHeight(), 0, Ogre::PF_BYTE_RGBA, Ogre::TU_DYNAMIC_WRITE_ONLY);
        }
        texture->unload();
        texture->setWidth(image.getWidth());
        texture->setHeight(image.getHeight());
        texture->loadImage(image);

        mScreenshot->setImageTexture(textureName);
    }
}
