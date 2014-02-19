#ifndef OPENMW_MWGUI_SAVEGAMEDIALOG_H
#define OPENMW_MWGUI_SAVEGAMEDIALOG_H

#include "windowbase.hpp"

namespace MWState
{
    class Character;
}

namespace MWGui
{

    class SaveGameDialog : public MWGui::WindowModal
    {
    public:
        SaveGameDialog();

        virtual void open();

        void setLoadOrSave(bool load);

    private:
        void onCancelButtonClicked (MyGUI::Widget* sender);
        void onOkButtonClicked (MyGUI::Widget* sender);
        void onCharacterSelected (MyGUI::ComboBox* sender, size_t pos);
        void onSlotSelected (MyGUI::ListBox* sender, size_t pos);
        void onSlotActivated (MyGUI::ListBox* sender, size_t pos);
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
        MyGUI::ListBox* mSaveList;
        MyGUI::EditBox* mSaveNameEdit;
        MyGUI::Widget* mSpacer;

        const MWState::Character* mCurrentCharacter;

    };

}

#endif
