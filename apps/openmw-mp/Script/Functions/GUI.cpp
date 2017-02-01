//
// Created by koncord on 23.07.16.
//

#include "GUI.hpp"
#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <apps/openmw-mp/Networking.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>



void GUIFunctions::_MessageBox(unsigned short pid, int id, const char *label) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->guiMessageBox.id = id;
    player->guiMessageBox.label = label;
    player->guiMessageBox.type = Player::GUIMessageBox::MessageBox;

    mwmp::Networking::get().getPlayerController()->GetPacket(ID_GUI_MESSAGEBOX)->Send(player, false);
}

void GUIFunctions::CustomMessageBox(unsigned short pid, int id, const char *label, const char *buttons) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->guiMessageBox.id = id;
    player->guiMessageBox.label = label;
    player->guiMessageBox.buttons = buttons;
    player->guiMessageBox.type = Player::GUIMessageBox::CustomMessageBox;

    mwmp::Networking::get().getPlayerController()->GetPacket(ID_GUI_MESSAGEBOX)->Send(player, false);
}

void GUIFunctions::InputDialog(unsigned short pid, int id, const char *label) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->guiMessageBox.id = id;
    player->guiMessageBox.label = label;
    player->guiMessageBox.type = Player::GUIMessageBox::InputDialog;

    mwmp::Networking::get().getPlayerController()->GetPacket(ID_GUI_MESSAGEBOX)->Send(player, false);
}

void GUIFunctions::ListBox(unsigned short pid, int id, const char *label, const char *items)
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->guiMessageBox.id = id;
    player->guiMessageBox.label = label;
    player->guiMessageBox.data = items;
    player->guiMessageBox.type = Player::GUIMessageBox::ListBox;

    mwmp::Networking::get().getPlayerController()->GetPacket(ID_GUI_MESSAGEBOX)->Send(player, false);
}

void GUIFunctions::SetMapVisibility(unsigned short targetPID, unsigned short affectedPID, unsigned short state) noexcept
{
    LOG_MESSAGE(Log::LOG_WARN, "stub");
}

void GUIFunctions::SetMapVisibilityAll(unsigned short targetPID, unsigned short state) noexcept
{
    LOG_MESSAGE(Log::LOG_WARN, "stub");
}

void GUIFunctions::SetConsoleAllow(unsigned short pid, bool state)
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->consoleAllowed = state;

    mwmp::Networking::get().getPlayerController()->GetPacket(ID_GAME_CONSOLE)->Send(player, false);
}
