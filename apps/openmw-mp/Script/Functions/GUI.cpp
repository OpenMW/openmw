//
// Created by koncord on 23.07.16.
//

#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <apps/openmw-mp/Networking.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>



void ScriptFunctions::MessageBox(unsigned short pid, int id, const char *label) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->guiMessageBox.id = id;
    player->guiMessageBox.label = label;
    player->guiMessageBox.type = Player::GUIMessageBox::MessageBox;

    mwmp::Networking::Get().GetController()->GetPacket(ID_GUI_MESSAGEBOX)->Send(player, false);
}

void ScriptFunctions::CustomMessageBox(unsigned short pid, int id, const char *label, const char *buttons) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->guiMessageBox.id = id;
    player->guiMessageBox.label = label;
    player->guiMessageBox.buttons = buttons;
    player->guiMessageBox.type = Player::GUIMessageBox::CustomMessageBox;

    mwmp::Networking::Get().GetController()->GetPacket(ID_GUI_MESSAGEBOX)->Send(player, false);
}

void ScriptFunctions::InputDialog(unsigned short pid, int id, const char *label) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->guiMessageBox.id = id;
    player->guiMessageBox.label = label;
    player->guiMessageBox.type = Player::GUIMessageBox::InputDialog;

    mwmp::Networking::Get().GetController()->GetPacket(ID_GUI_MESSAGEBOX)->Send(player, false);
}