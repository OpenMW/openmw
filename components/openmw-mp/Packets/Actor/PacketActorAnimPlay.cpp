#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Log.hpp>
#include "PacketActorAnimPlay.hpp"

using namespace mwmp;

PacketActorAnimPlay::PacketActorAnimPlay(RakNet::RakPeerInterface *peer) : ActorPacket(peer)
{
    packetID = ID_ACTOR_ANIM_PLAY;
}

void PacketActorAnimPlay::Actor(BaseActor &actor, bool send)
{

    RW(actor.animation.groupname, send);
    RW(actor.animation.mode, send);
    RW(actor.animation.count, send);
    RW(actor.animation.persist, send);
}
