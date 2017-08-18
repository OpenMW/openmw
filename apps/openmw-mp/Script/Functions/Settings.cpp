#include "Settings.hpp"

#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Log.hpp>

#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <apps/openmw-mp/Networking.hpp>

#include <iostream>
using namespace std;

void SettingFunctions::SetConsoleAllow(unsigned short pid, bool state)
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->consoleAllowed = state;
}

void SettingFunctions::SetDifficulty(unsigned short pid, int difficulty)
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->difficulty = difficulty;
}

void SettingFunctions::SendSettings(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_GAME_SETTINGS)->setPlayer(player);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_GAME_SETTINGS)->Send(false);
}
