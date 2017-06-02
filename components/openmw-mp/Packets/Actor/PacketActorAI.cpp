#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Log.hpp>
#include "PacketActorAI.hpp"

using namespace mwmp;

PacketActorAI::PacketActorAI(RakNet::RakPeerInterface *peer) : ActorPacket(peer)
{
    packetID = ID_ACTOR_AI;
}

void PacketActorAI::Actor(BaseActor &actor, bool send)
{
    // Placeholder to be filled in later
}
