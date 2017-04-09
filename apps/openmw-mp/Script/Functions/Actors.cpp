#include <regex>

#include <apps/openmw-mp/Player.hpp>
#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <apps/openmw-mp/Networking.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Base/BaseActor.hpp>
#include "Actors.hpp"

using namespace mwmp;

ActorList actorList;
BaseActor tempActor;

void ActorFunctions::InitActorList(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    actorList.cell.blank();
    actorList.baseActors.clear();
    actorList.guid = player->guid;
}

unsigned int ActorFunctions::GetActorListSize() noexcept
{
    return mwmp::Networking::getPtr()->getLastEvent()->objectChanges.count;
}
