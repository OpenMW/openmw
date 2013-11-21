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

        void onCancelButtonClicked (MyGUI::Widget* sender);
        void onOkButtonClicked (MyGUI::Widget* sender);
        void onCharacterSelected (MyGUI::ComboBox* sender, size_t pos);
        void onSlotSelected (MyGUI::ListBox* sender, size_t pos);

        void fillSaveList();


    private:
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
