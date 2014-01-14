#ifndef OPENMW_MWGUI_SAVEGAMEDIALOG_H
#define OPENMW_MWGUI_SAVEGAMEDIALOG_H

#include "windowbase.hpp"

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


    private:
        MyGUI::ImageBox* mScreenshot;

        MyGUI::ComboBox* mCharacterSelection;
        MyGUI::EditBox* mInfoText;
        MyGUI::Button* mOkButton;
        MyGUI::Button* mCancelButton;
        MyGUI::ListBox* mSaveList;
        MyGUI::EditBox* mSaveNameEdit;
        MyGUI::Widget* mSpacer;

    };

}

#endif
