//
// Created by koncord on 03.11.16.
//

#include "GUIDialogList.hpp"
#include "Main.hpp"
#include "LocalPlayer.hpp"
#include "Networking.hpp"
#include <MyGUI_EditBox.h>
#include <MyGUI_Button.h>
#include <MyGUI_ListBox.h>

#include "../mwgui/windowmanagerimp.hpp"
#include "../mwbase/environment.hpp"

#include <components/openmw-mp/Log.hpp>

using namespace mwmp;

GUIDialogList::GUIDialogList(const std::string &message, const std::vector<std::string> &list) : WindowModal("tes3mp_dialog_list.layout")
{
    center(); // center window

    getWidget(mListBox, "ListBox");
    getWidget(mMessage, "Message");
    getWidget(mButton, "OkButton");

    mButton->eventMouseButtonClick += MyGUI::newDelegate(this, &GUIDialogList::mousePressed);

    mMessage->setCaptionWithReplacing(message);
    for(size_t i = 0; i < list.size(); i++)
        mListBox->addItem(list[i]);

}

void GUIDialogList::mousePressed(MyGUI::Widget * /*widget*/)
{
    setVisible(false);
    MWBase::Environment::get().getWindowManager()->popGuiMode();

    size_t id = mListBox->getIndexSelected();

    Main::get().getLocalPlayer()->guiMessageBox.data = MyGUI::utility::toString(id);
    Main::get().getNetworking()->getPlayerPacket(ID_GUI_MESSAGEBOX)->Send(Main::get().getLocalPlayer());

    LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Selected id: %d", id);
    if(id == MyGUI::ITEM_NONE)
        return;

    std::string itemName = mListBox->getItemNameAt(mListBox->getIndexSelected()).asUTF8();
    LOG_APPEND(Log::LOG_VERBOSE, "name of item: '%s'", itemName.c_str());
}

void GUIDialogList::onFrame(float frameDuration)
{

}
