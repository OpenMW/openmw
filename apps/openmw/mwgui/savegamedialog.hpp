#ifndef OPENMW_MWGUI_SAVEGAMEDIALOG_H
#define OPENMW_MWGUI_SAVEGAMEDIALOG_H

#include "windowbase.hpp"

namespace MWState
{
    class Character;
    class Slot;
}

namespace MWGui
{

    class SaveGameDialog : public MWGui::WindowModal
    {
    public:
        SaveGameDialog();

        virtual void open();

        virtual void exit();

        void setLoadOrSave(bool load);

    private:
        void confirmDeleteSave();

        void onCancelButtonClicked (MyGUI::Widget* sender);
        void onOkButtonClicked (MyGUI::Widget* sender);
        void onDeleteButtonClicked (MyGUI::Widget* sender);
        void onCharacterSelected (MyGUI::ComboBox* sender, size_t pos);
        // Slot selected (mouse click or arrow keys)
        void onSlotSelected (MyGUI::ListBox* sender, size_t pos);
        // Slot activated (double click or enter key)
        void onSlotActivated (MyGUI::ListBox* sender, size_t pos);
        // Slot clicked with mouse
        void onSlotMouseClick(MyGUI::ListBox* sender, size_t pos);

        void onDeleteSlotConfirmed();

        void onEditSelectAccept (MyGUI::EditBox* sender);
        void onSaveNameChanged (MyGUI::EditBox* sender);
        void onConfirmationGiven();

        void accept(bool reallySure=false);

        void fillSaveList();

        MyGUI::ImageBox* mScreenshot;
        bool mSaving;

        MyGUI::ComboBox* mCharacterSelection;
        MyGUI::EditBox* mInfoText;
        MyGUI::Button* mOkButton;
        MyGUI::Button* mCancelButton;
        MyGUI::Button* mDeleteButton;
        MyGUI::ListBox* mSaveList;
        MyGUI::EditBox* mSaveNameEdit;
        MyGUI::Widget* mSpacer;

        const MWState::Character* mCurrentCharacter;
        const MWState::Slot* mCurrentSlot;

    };

}

#endif
