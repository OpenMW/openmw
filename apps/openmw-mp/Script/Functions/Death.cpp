#include "Death.hpp"
#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <apps/openmw-mp/Networking.hpp>
#include <components/openmw-mp/Log.hpp>

#include <iostream>
using namespace std;

void DeathFunctions::SetResurrectType(unsigned short pid, unsigned int type)
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->resurrectType = type;
}

void DeathFunctions::SendResurrect(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_RESURRECT)->setPlayer(player);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_RESURRECT)->Send(false);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_RESURRECT)->Send(true);
}
