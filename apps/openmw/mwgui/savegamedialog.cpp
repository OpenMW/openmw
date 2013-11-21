#include "savegamedialog.hpp"
#include "widgets.hpp"

#include "../mwbase/statemanager.hpp"
#include "../mwbase/environment.hpp"


#include "../mwstate/character.hpp"

namespace
{
std::string getMonth(int m)
{
    std::string month;
    switch (m) {
        case 0:
            month = "#{sMonthMorningstar}";
            break;
        case 1:
            month = "#{sMonthSunsdawn}";
            break;
        case 2:
            month = "#{sMonthFirstseed}";
            break;
        case 3:
            month = "#{sMonthRainshand}";
            break;
        case 4:
            month = "#{sMonthSecondseed}";
            break;
        case 5:
            month = "#{sMonthMidyear}";
            break;
        case 6:
            month = "#{sMonthSunsheight}";
            break;
        case 7:
            month = "#{sMonthLastseed}";
            break;
        case 8:
            month = "#{sMonthHeartfire}";
            break;
        case 9:
            month = "#{sMonthFrostfall}";
            break;
        case 10:
            month = "#{sMonthSunsdusk}";
            break;
        case 11:
            month = "#{sMonthEveningstar}";
            break;
        default:
            break;
    }
    return month;
}
}

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

    }

    void SaveGameDialog::open()
    {
        WindowModal::open();

        center();

        MWBase::StateManager* mgr = MWBase::Environment::get().getStateManager();
        if (mgr->characterBegin() == mgr->characterEnd())
            return;

        // If we are running, there must be a current character
        if (mgr->getState() == MWBase::StateManager::State_Running)
        {
            mCurrentCharacter = mgr->getCurrentCharacter();
        }

        mCharacterSelection->removeAllItems();
        for (MWBase::StateManager::CharacterIterator it = mgr->characterBegin(); it != mgr->characterEnd(); ++it)
        {
            std::stringstream title;
            title << it->getSignature().mPlayerName;
            title << " (Level " << it->getSignature().mPlayerLevel << " " << it->getSignature().mPlayerClass << ")";

            mCharacterSelection->addItem (title.str());

            if (mCurrentCharacter == &*it)
                mCharacterSelection->setIndexSelected(mCharacterSelection->getItemCount()-1);
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
            mCurrentCharacter = MWBase::Environment::get().getStateManager()->getCurrentCharacter();
        }

        center();
    }

    void SaveGameDialog::onCancelButtonClicked(MyGUI::Widget *sender)
    {
        setVisible(false);
    }

    void SaveGameDialog::onOkButtonClicked(MyGUI::Widget *sender)
    {
        // Get the selected slot, if any
        unsigned int i=0;
        const MWState::Slot* slot = NULL;
        for (MWState::Character::SlotIterator it = mCurrentCharacter->begin(); it != mCurrentCharacter->end(); ++it,++i)
        {
            if (i == mSaveList->getIndexSelected())
                slot = &*it;
        }

        if (mSaving)
        {
            MWBase::Environment::get().getStateManager()->saveGame (slot);
        }
        else
        {
            MWBase::Environment::get().getStateManager()->loadGame (mCurrentCharacter, slot);
        }

        setVisible(false);
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
            mSaveList->addItem(it->mPath.string());
        }
        onSlotSelected(mSaveList, MyGUI::ITEM_NONE);
    }

    void SaveGameDialog::onSlotSelected(MyGUI::ListBox *sender, size_t pos)
    {
        if (pos == MyGUI::ITEM_NONE)
        {
            mInfoText->setCaption("");
            return;
        }

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

        text << asctime(timeinfo) << "\n";
        text << "Level " << slot->mProfile.mPlayerLevel << "\n";
        text << slot->mProfile.mPlayerCell << "\n";
        //text << "Time played: " << slot->mProfile.mTimePlayed << "\n";

        int hour = int(slot->mProfile.mInGameTime.mGameHour);
        bool pm = hour >= 12;
        if (hour >= 13) hour -= 12;
        if (hour == 0) hour = 12;

        text <<
                slot->mProfile.mInGameTime.mDay << " "
               << getMonth(slot->mProfile.mInGameTime.mMonth)
               <<  " " << hour << " " << (pm ? "#{sSaveMenuHelp05}" : "#{sSaveMenuHelp04}");

        mInfoText->setCaptionWithReplacing(text.str());

    }
}
