//
// Created by koncord on 20.07.16.
//

#include <SDL_system.h>
#include <MyGUI_InputManager.h>
#include <apps/openmw/mwbase/windowmanager.hpp>
#include <apps/openmw/mwbase/inputmanager.hpp>
#include <apps/openmw/mwbase/environment.hpp>
#include <components/openmw-mp/Base/BasePlayer.hpp>


#include "GUIController.hpp"
#include "Main.hpp"


mwmp::GUIController::GUIController(): mInputBox(0)
{
    mChat = nullptr;
    keySay = SDLK_y;
    keyChatMode = SDLK_F2;
    calledMessageBox = false;
}

mwmp::GUIController::~GUIController()
{
   /* if(mChat != nullptr)
        delete mChat;
    mChat = nullptr;*/
}

void mwmp::GUIController::setupChat(const Settings::Manager &mgr)
{
    assert(mChat == nullptr);

    float chatDelay = mgr.getFloat("delay", "Chat");
    int chatY = mgr.getInt("y", "Chat");
    int chatX = mgr.getInt("x", "Chat");
    int chatW = mgr.getInt("w", "Chat");
    int chatH = mgr.getInt("h", "Chat");

    keySay =      SDL_GetKeyFromName(mgr.getString("keySay", "Chat").c_str());
    keyChatMode = SDL_GetKeyFromName(mgr.getString("keyChatMode", "Chat").c_str());

    mChat = new GUIChat(chatX, chatY, chatW, chatH);
    mChat->SetDelay(chatDelay);
}

void mwmp::GUIController::PrintChatMessage(std::string &msg)
{
    if (mChat != nullptr)
        mChat->print(msg);
}


void mwmp::GUIController::setChatVisible(bool chatVisible)
{
    mChat->setVisible(chatVisible);
}

void mwmp::GUIController::ShowMessageBox(const BasePlayer::GUIMessageBox &guiMessageBox)
{
    MWBase::WindowManager *windowManager = MWBase::Environment::get().getWindowManager();
    std::vector<std::string> buttons;
    buttons.push_back("Ok");
    windowManager->interactiveMessageBox(guiMessageBox.label, buttons);
    calledMessageBox = true;
}

std::vector<std::string> splitString(const std::string &str, char delim = ';')
{
    std::istringstream ss(str);
    std::vector<std::string> result;
    std::string token;
    while (std::getline(ss, token, delim))
        result.push_back(token);
    return result;
}

void mwmp::GUIController::ShowCustomMessageBox(const BasePlayer::GUIMessageBox &guiMessageBox)
{
    MWBase::WindowManager *windowManager = MWBase::Environment::get().getWindowManager();
    std::vector<std::string> buttons = splitString(guiMessageBox.buttons);
    windowManager->interactiveMessageBox(guiMessageBox.label, buttons);
    calledMessageBox = true;
}

void mwmp::GUIController::ShowInputBox(const BasePlayer::GUIMessageBox &guiMessageBox)
{
    printf("test adf\n");
    MWBase::WindowManager *windowManager = MWBase::Environment::get().getWindowManager();

    windowManager->removeDialog(mInputBox);
    mInputBox = 0;
    mInputBox = new MWGui::TextInputDialog();
    mInputBox->setTextLabel(guiMessageBox.label);
    mInputBox->eventDone += MyGUI::newDelegate(this, &GUIController::OnInputBoxDone);
    mInputBox->setVisible(true);

}

void mwmp::GUIController::OnInputBoxDone(MWGui::WindowBase *parWindow)
{
    //MWBase::WindowManager *windowManager = MWBase::Environment::get().getWindowManager();
    printf("GUIController::OnInputBoxDone: %s.\n",mInputBox->getTextInput().c_str());

    Main::get().getLocalPlayer()->guiMessageBox.data = mInputBox->getTextInput();
    Main::get().getNetworking()->GetPacket(ID_GUI_MESSAGEBOX)->Send(Main::get().getLocalPlayer());

    MWBase::Environment::get().getWindowManager()->removeDialog(mInputBox);
    mInputBox = 0;
}

bool mwmp::GUIController::pressedKey(int key)
{
    MWBase::WindowManager *windowManager = MWBase::Environment::get().getWindowManager();
    if (mChat == nullptr || windowManager->getMode() != MWGui::GM_None)
        return false;
    if (key == keyChatMode)
    {
        mChat->PressedChatMode();
        return true;
    }
    else if (key == keySay)
    {
        //MyGUI::Widget *oldFocus = MyGUI::InputManager::getInstance().getKeyFocusWidget();
        mChat->PressedSay();
        /*MyGUI::Widget *newFocus = MyGUI::InputManager::getInstance().getKeyFocusWidget();
        printf("mwmp::GUIController::pressedKey. oldFocus: %s.\n", oldFocus ? oldFocus->getName().c_str() : "nil");
        printf("mwmp::GUIController::pressedKey.newFocus: %s.\n", newFocus ? newFocus->getName().c_str() : "nil");*/
        return true;
    }
    return false;
}

bool mwmp::GUIController::HaveFocusedElement()
{
    return false;
}


void mwmp::GUIController::update(float dt)
{
    if (mChat != nullptr)
        mChat->Update(dt);

    int pressedButton = MWBase::Environment::get().getWindowManager()->readPressedButton();
    if (pressedButton != -1 && calledMessageBox)
    {
        printf("Pressed: %d\n", pressedButton);
        calledMessageBox = false;
        Main::get().getLocalPlayer()->guiMessageBox.data = MyGUI::utility::toString(pressedButton);
        Main::get().getNetworking()->GetPacket(ID_GUI_MESSAGEBOX)->Send(Main::get().getLocalPlayer());
    }

}


