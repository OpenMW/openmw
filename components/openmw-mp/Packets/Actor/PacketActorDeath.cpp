#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Log.hpp>
#include "PacketActorDeath.hpp"

using namespace mwmp;

PacketActorDeath::PacketActorDeath(RakNet::RakPeerInterface *peer) : ActorPacket(peer)
{
    packetID = ID_ACTOR_DEATH;
}

void PacketActorDeath::Actor(BaseActor &actor, bool send)
{
        // Placeholder to be filled in later
}
