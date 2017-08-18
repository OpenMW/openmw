#include "Mechanics.hpp"

#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Log.hpp>

#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <apps/openmw-mp/Networking.hpp>

#include <iostream>
using namespace std;

bool MechanicsFunctions::IsWerewolf(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->isWerewolf;
}

void MechanicsFunctions::SetWerewolfState(unsigned short pid, bool isWerewolf)
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->isWerewolf = isWerewolf;
}

void MechanicsFunctions::SendShapeshift(unsigned short pid)
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_SHAPESHIFT)->setPlayer(player);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_SHAPESHIFT)->Send(false);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_SHAPESHIFT)->Send(true);
}

void MechanicsFunctions::Jail(unsigned short pid, int jailDays, bool ignoreJailTeleportation, bool ignoreJailSkillIncreases,
                              const char* jailProgressText, const char* jailEndText) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->jailDays = jailDays;
    player->ignoreJailTeleportation = ignoreJailTeleportation;
    player->ignoreJailSkillIncreases = ignoreJailSkillIncreases;
    player->jailProgressText = jailProgressText;
    player->jailEndText = jailEndText;

    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_JAIL)->setPlayer(player);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_JAIL)->Send(false);
}

void MechanicsFunctions::Resurrect(unsigned short pid, unsigned int type) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->resurrectType = type;

    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_RESURRECT)->setPlayer(player);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_RESURRECT)->Send(false);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_RESURRECT)->Send(true);
}
