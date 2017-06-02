#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Log.hpp>
#include <components/esm/creaturestats.hpp>
#include "PacketActorStatsDynamic.hpp"

using namespace mwmp;

PacketActorStatsDynamic::PacketActorStatsDynamic(RakNet::RakPeerInterface *peer) : ActorPacket(peer)
{
    packetID = ID_ACTOR_STATS_DYNAMIC;
}

void PacketActorStatsDynamic::Actor(BaseActor &actor, bool send)
{
    RW(actor.creatureStats.mDynamic, send);

    actor.hasStatsDynamicData = true;
}
