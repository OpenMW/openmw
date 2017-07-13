#include "Mechanics.hpp"
#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <apps/openmw-mp/Networking.hpp>
#include <components/openmw-mp/Log.hpp>

#include <iostream>
using namespace std;

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
