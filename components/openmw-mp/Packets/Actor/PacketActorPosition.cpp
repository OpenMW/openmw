#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Log.hpp>
#include "PacketActorPosition.hpp"

using namespace mwmp;

PacketActorPosition::PacketActorPosition(RakNet::RakPeerInterface *peer) : ActorPacket(peer)
{
    packetID = ID_ACTOR_POSITION;
}

void PacketActorPosition::Actor(BaseActor &actor, bool send)
{
    RW(actor.position, send, 1);
    RW(actor.direction, send, 1);

    actor.hasPositionData = true;
}
