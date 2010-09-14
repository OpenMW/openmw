#include "text_input.hpp"
#include "../mwworld/environment.hpp"
#include "../mwworld/world.hpp"

using namespace MWGui;

TextInputDialog::TextInputDialog(MWWorld::Environment& environment, const std::string &label, bool showNext, MyGUI::IntSize size)
  : Layout("openmw_text_input_layout.xml")
  , environment(environment)
{
    // Centre dialog
    MyGUI::IntCoord coord = mMainWidget->getCoord();
    coord.left = (size.width - coord.width)/2;
    coord.top = (size.height - coord.height)/2;
    mMainWidget->setCoord(coord);

    setText("LabelT", label);

    getWidget(textEdit, "TextEdit");
//    textEdit->eventEditSelectAccept = newDelegate(this, &TextInputDialog::onTextAccepted);

    // TODO: These buttons should be managed by a Dialog class
    MyGUI::ButtonPtr okButton;
    getWidget(okButton, "OKButton");
    okButton->eventMouseButtonClick = MyGUI::newDelegate(this, &TextInputDialog::onOkClicked);
    if (showNext)
    {
        okButton->setCaption("Next");

        // Adjust back button when next is shown
        okButton->setCoord(okButton->getCoord() + MyGUI::IntCoord(-18, 0, 18, 0));
    }
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
