#include "text_input.hpp"
#include "window_manager.hpp"

using namespace MWGui;

TextInputDialog::TextInputDialog(WindowManager& parWindowManager)
  : WindowBase("openmw_text_input_layout.xml", parWindowManager)
{
    // Centre dialog
    center();

    getWidget(textEdit, "TextEdit");
    textEdit->eventEditSelectAccept = newDelegate(this, &TextInputDialog::onTextAccepted);

    // TODO: These buttons should be managed by a Dialog class
    MyGUI::ButtonPtr okButton;
    getWidget(okButton, "OKButton");
    okButton->eventMouseButtonClick = MyGUI::newDelegate(this, &TextInputDialog::onOkClicked);

    // Make sure the edit box has focus
    MyGUI::InputManager::getInstance().setKeyFocusWidget(textEdit);
}

void TextInputDialog::setNextButtonShow(bool shown)
{
    MyGUI::ButtonPtr okButton;
    getWidget(okButton, "OKButton");
    if (shown)
    {
        okButton->setCaption("Next");
        okButton->setCoord(MyGUI::IntCoord(264 - 18, 60, 42 + 18, 23));
    }
    else
    {
        okButton->setCaption("OK");
        okButton->setCoord(MyGUI::IntCoord(264, 60, 42, 23));
    }
}

void TextInputDialog::setTextLabel(const std::string &label)
{
    setText("LabelT", label);
}

void TextInputDialog::open()
{
    // Make sure the edit box has focus
    MyGUI::InputManager::getInstance().setKeyFocusWidget(textEdit);
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
