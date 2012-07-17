#include "text_input.hpp"
#include "window_manager.hpp"

using namespace MWGui;

TextInputDialog::TextInputDialog(WindowManager& parWindowManager)
  : WindowBase("openmw_text_input.layout", parWindowManager)
{
    // Centre dialog
    center();

    getWidget(mTextEdit, "TextEdit");
    mTextEdit->eventEditSelectAccept += newDelegate(this, &TextInputDialog::onTextAccepted);

    MyGUI::ButtonPtr okButton;
    getWidget(okButton, "OKButton");
    okButton->eventMouseButtonClick += MyGUI::newDelegate(this, &TextInputDialog::onOkClicked);

    // Make sure the edit box has focus
    MyGUI::InputManager::getInstance().setKeyFocusWidget(mTextEdit);
}

void TextInputDialog::setNextButtonShow(bool shown)
{
    MyGUI::ButtonPtr okButton;
    getWidget(okButton, "OKButton");

    if (shown)
        okButton->setCaption(mWindowManager.getGameSettingString("sNext", ""));
    else
        okButton->setCaption(mWindowManager.getGameSettingString("sOK", ""));

    int okButtonWidth = okButton->getTextSize().width + 24;

    okButton->setCoord(306 - okButtonWidth, 60, okButtonWidth, 23);
}

void TextInputDialog::setTextLabel(const std::string &label)
{
    setText("LabelT", label);
}

void TextInputDialog::open()
{
    // Make sure the edit box has focus
    MyGUI::InputManager::getInstance().setKeyFocusWidget(mTextEdit);
    setVisible(true);
}

// widget controls

void TextInputDialog::onOkClicked(MyGUI::Widget* _sender)
{
    eventDone(this);
}

void TextInputDialog::onTextAccepted(MyGUI::Edit* _sender)
{
    eventDone(this);
}
