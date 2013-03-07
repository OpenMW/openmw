#include "text_input.hpp"

#include "../mwbase/windowmanager.hpp"

using namespace MWGui;

TextInputDialog::TextInputDialog(MWBase::WindowManager& parWindowManager)
  : WindowModal("openmw_text_input.layout", parWindowManager)
{
    // Centre dialog
    center();

    getWidget(mTextEdit, "TextEdit");
    mTextEdit->eventEditSelectAccept += newDelegate(this, &TextInputDialog::onTextAccepted);

    MyGUI::Button* okButton;
    getWidget(okButton, "OKButton");
    okButton->eventMouseButtonClick += MyGUI::newDelegate(this, &TextInputDialog::onOkClicked);

    // Make sure the edit box has focus
    MyGUI::InputManager::getInstance().setKeyFocusWidget(mTextEdit);
}

void TextInputDialog::setNextButtonShow(bool shown)
{
    MyGUI::Button* okButton;
    getWidget(okButton, "OKButton");

    if (shown)
        okButton->setCaption(mWindowManager.getGameSettingString("sNext", ""));
    else
        okButton->setCaption(mWindowManager.getGameSettingString("sOK", ""));
}

void TextInputDialog::setTextLabel(const std::string &label)
{
    setText("LabelT", label);
}

void TextInputDialog::open()
{
    WindowModal::open();
    // Make sure the edit box has focus
    MyGUI::InputManager::getInstance().setKeyFocusWidget(mTextEdit);
}

// widget controls

void TextInputDialog::onOkClicked(MyGUI::Widget* _sender)
{
    if (mTextEdit->getCaption() == "")
    {
        mWindowManager.messageBox ("#{sNotifyMessage37}", std::vector<std::string>());
        MyGUI::InputManager::getInstance ().setKeyFocusWidget (mTextEdit);
    }
    else
        eventDone(this);
}

void TextInputDialog::onTextAccepted(MyGUI::Edit* _sender)
{
    if (mTextEdit->getCaption() == "")
    {
        mWindowManager.messageBox ("#{sNotifyMessage37}", std::vector<std::string>());
        MyGUI::InputManager::getInstance ().setKeyFocusWidget (mTextEdit);
    }
    else
        eventDone(this);
}
