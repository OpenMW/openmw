#include "savegamedialog.hpp"
#include "widgets.hpp"

#include <OgreImage.h>
#include <OgreTextureManager.h>

#include <components/misc/stringops.hpp>

#include <components/settings/settings.hpp>

#include "../mwbase/statemanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwstate/character.hpp"

#include "confirmationdialog.hpp"

namespace MWGui
{
    SaveGameDialog::SaveGameDialog()
        : WindowModal("openmw_savegame_dialog.layout")
        , mSaving(true)
        , mCurrentCharacter(NULL)
    {
        getWidget(mScreenshot, "Screenshot");
        getWidget(mCharacterSelection, "SelectCharacter");
        getWidget(mInfoText, "InfoText");
        getWidget(mOkButton, "OkButton");
        getWidget(mCancelButton, "CancelButton");
        getWidget(mSaveList, "SaveList");
        getWidget(mSaveNameEdit, "SaveNameEdit");
        getWidget(mSpacer, "Spacer");
        mOkButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SaveGameDialog::onOkButtonClicked);
        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SaveGameDialog::onCancelButtonClicked);
        mCharacterSelection->eventComboChangePosition += MyGUI::newDelegate(this, &SaveGameDialog::onCharacterSelected);
        mSaveList->eventListChangePosition += MyGUI::newDelegate(this, &SaveGameDialog::onSlotSelected);
        mSaveList->eventListSelectAccept += MyGUI::newDelegate(this, &SaveGameDialog::onSlotActivated);
        mSaveNameEdit->eventEditSelectAccept += MyGUI::newDelegate(this, &SaveGameDialog::onEditSelectAccept);
        mSaveNameEdit->eventEditTextChange += MyGUI::newDelegate(this, &SaveGameDialog::onSaveNameChanged);
    }

    void SaveGameDialog::onSlotActivated(MyGUI::ListBox *sender, size_t pos)
    {
        onSlotSelected(sender, pos);
        accept();
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

        MWBase::StateManager* mgr = MWBase::Environment::get().getStateManager();
        if (mgr->characterBegin() == mgr->characterEnd())
            return;

        mCurrentCharacter = mgr->getCurrentCharacter (false);

        std::string directory =
            Misc::StringUtils::lowerCase (Settings::Manager::getString ("character", "Saves"));

        mCharacterSelection->removeAllItems();

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
                    const ESM::Class* class_ = MWBase::Environment::get().getWorld()->getStore().get<ESM::Class>().find(
                                it->getSignature().mPlayerClassId);
                    className = class_->mName;
                }

                title << " (Level " << it->getSignature().mPlayerLevel << " " << className << ")";

                mCharacterSelection->addItem (title.str());

                if (mCurrentCharacter == &*it ||
                    (!mCurrentCharacter && !mSaving && directory==Misc::StringUtils::lowerCase (
                    it->begin()->mPath.parent_path().filename().string())))
                {
                    mCurrentCharacter = &*it;
                    mCharacterSelection->setIndexSelected(mCharacterSelection->getItemCount()-1);
                }
            }
        }

        fillSaveList();

    }

    void SaveGameDialog::setLoadOrSave(bool load)
    {
        mSaving = !load;
        mSaveNameEdit->setVisible(!load);
        mCharacterSelection->setUserString("Hidden", load ? "false" : "true");
        mCharacterSelection->setVisible(load);
        mSpacer->setUserString("Hidden", load ? "false" : "true");

        if (!load)
        {
            mCurrentCharacter = MWBase::Environment::get().getStateManager()->getCurrentCharacter (false);
        }

        center();
    }

    void SaveGameDialog::onCancelButtonClicked(MyGUI::Widget *sender)
    {
        setVisible(false);
    }

    void SaveGameDialog::onConfirmationGiven()
    {
        accept(true);
    }

    void SaveGameDialog::accept(bool reallySure)
    {
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(NULL);

        // Get the selected slot, if any
        unsigned int i=0;
        const MWState::Slot* slot = NULL;

        if (mCurrentCharacter)
        {
            for (MWState::Character::SlotIterator it = mCurrentCharacter->begin(); it != mCurrentCharacter->end(); ++it,++i)
            {
                if (i == mSaveList->getIndexSelected())
                    slot = &*it;
            }
        }

        if (mSaving)
        {
            // If overwriting an existing slot, ask for confirmation first
            if (slot != NULL && !reallySure)
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
            MWBase::Environment::get().getStateManager()->saveGame (mSaveNameEdit->getCaption(), slot);
        }
        else
        {
            if (mCurrentCharacter && slot)
            {
                MWBase::Environment::get().getStateManager()->loadGame (mCurrentCharacter, slot);
                MWBase::Environment::get().getWindowManager()->removeGuiMode (MWGui::GM_MainMenu);
            }
        }

        setVisible(false);

        if (MWBase::Environment::get().getStateManager()->getState()==
            MWBase::StateManager::State_NoGame)
        {
            MWBase::Environment::get().getWindowManager()->pushGuiMode (MWGui::GM_MainMenu);
        }
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
        onSlotSelected(mSaveList, MyGUI::ITEM_NONE);
    }

    void SaveGameDialog::onSlotSelected(MyGUI::ListBox *sender, size_t pos)
    {
        if (pos == MyGUI::ITEM_NONE)
        {
            mInfoText->setCaption("");
            mScreenshot->setImageTexture("");
            return;
        }

        if (mSaving)
            mSaveNameEdit->setCaption(sender->getItemNameAt(pos));

        const MWState::Slot* slot = NULL;
        unsigned int i=0;
        for (MWState::Character::SlotIterator it = mCurrentCharacter->begin(); it != mCurrentCharacter->end(); ++it, ++i)
        {
            if (i == pos)
                slot = &*it;
        }
        assert(slot && "Can't find selected slot");

        std::stringstream text;
        time_t time = slot->mTimeStamp;
        struct tm* timeinfo;
        timeinfo = localtime(&time);

        // Use system/environment locale settings for datetime formatting
        std::setlocale(LC_TIME, "");

        const int size=1024;
        char buffer[size];
        if (std::strftime(buffer, size, "%x %X", timeinfo) > 0)
            text << buffer << "\n";
        text << "Level " << slot->mProfile.mPlayerLevel << "\n";
        text << slot->mProfile.mPlayerCell << "\n";
        // text << "Time played: " << slot->mProfile.mTimePlayed << "\n";

        int hour = int(slot->mProfile.mInGameTime.mGameHour);
        bool pm = hour >= 12;
        if (hour >= 13) hour -= 12;
        if (hour == 0) hour = 12;

        text
            << slot->mProfile.mInGameTime.mDay << " "
            << MWBase::Environment::get().getWorld()->getMonthName(slot->mProfile.mInGameTime.mMonth)
            <<  " " << hour << " " << (pm ? "#{sSaveMenuHelp05}" : "#{sSaveMenuHelp04}");

        mInfoText->setCaptionWithReplacing(text.str());

        // Decode screenshot
        std::vector<char> data = slot->mProfile.mScreenshot; // MemoryDataStream doesn't work with const data :(
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
