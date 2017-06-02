#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Log.hpp>
#include "PacketActorAnimFlags.hpp"

using namespace mwmp;

PacketActorAnimFlags::PacketActorAnimFlags(RakNet::RakPeerInterface *peer) : ActorPacket(peer)
{
    packetID = ID_ACTOR_ANIM_FLAGS;
}

void PacketActorAnimFlags::Actor(BaseActor &actor, bool send)
{
    RW(actor.movementFlags, send);
    RW(actor.drawState, send);
    RW(actor.isFlying, send);
}
