#include "savegamedialog.hpp"
#include "widgets.hpp"


namespace MWGui
{

    SaveGameDialog::SaveGameDialog()
        : WindowModal("openmw_savegame_dialog.layout")
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

    }

    void SaveGameDialog::open()
    {
        center();
    }

    void SaveGameDialog::setLoadOrSave(bool load)
    {
        mSaveNameEdit->setVisible(!load);
        mCharacterSelection->setUserString("Hidden", load ? "false" : "true");
        mCharacterSelection->setVisible(load);
        mSpacer->setUserString("Hidden", load ? "false" : "true");

        center();
    }

    void SaveGameDialog::onCancelButtonClicked(MyGUI::Widget *sender)
    {
        setVisible(false);
    }

    void SaveGameDialog::onOkButtonClicked(MyGUI::Widget *sender)
    {
        setVisible(false);
    }

}
