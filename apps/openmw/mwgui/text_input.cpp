#include "text_input.hpp"
#include "../mwworld/environment.hpp"
#include "../mwworld/world.hpp"

using namespace MWGui;

TextInputDialog::TextInputDialog(MWWorld::Environment& environment, MyGUI::IntSize size)
  : Layout("openmw_text_input_layout.xml")
  , environment(environment)
{
    // Centre dialog
    MyGUI::IntCoord coord = mMainWidget->getCoord();
    coord.left = (size.width - coord.width)/2;
    coord.top = (size.height - coord.height)/2;
    mMainWidget->setCoord(coord);

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
    eventDone();
}

void TextInputDialog::onTextAccepted(MyGUI::Edit* _sender)
{
    eventDone();
}
