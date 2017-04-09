#include <regex>

#include <apps/openmw-mp/Player.hpp>
#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <apps/openmw-mp/Networking.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Base/BaseActor.hpp>
#include "Actors.hpp"

using namespace mwmp;

BaseActorList scriptActorList;
BaseActor tempActor;

void ActorFunctions::InitActorList(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    scriptActorList.cell.blank();
    scriptActorList.baseActors.clear();
    scriptActorList.guid = player->guid;
}

unsigned int ActorFunctions::GetActorListSize() noexcept
{
    return mwmp::Networking::getPtr()->getLastEvent()->objectChanges.count;
}

void ActorFunctions::SendActorList() noexcept
{
    mwmp::Networking::get().getActorPacketController()->GetPacket(ID_ACTOR_LIST)->setActorList(&scriptActorList);
    mwmp::Networking::get().getActorPacketController()->GetPacket(ID_ACTOR_LIST)->Send(scriptActorList.guid);
}

void ActorFunctions::SendActorAuthority() noexcept
{
    mwmp::Networking::get().getActorPacketController()->GetPacket(ID_ACTOR_AUTHORITY)->setActorList(&scriptActorList);
    mwmp::Networking::get().getActorPacketController()->GetPacket(ID_ACTOR_AUTHORITY)->Send(scriptActorList.guid);
}

