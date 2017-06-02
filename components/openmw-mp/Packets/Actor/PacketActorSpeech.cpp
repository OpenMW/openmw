#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Log.hpp>
#include "PacketActorSpeech.hpp"

using namespace mwmp;

PacketActorSpeech::PacketActorSpeech(RakNet::RakPeerInterface *peer) : ActorPacket(peer)
{
    packetID = ID_ACTOR_SPEECH;
}

void PacketActorSpeech::Actor(BaseActor &actor, bool send)
{
    RW(actor.response, send);
    RW(actor.sound, send);
}
