#include <components/openmw-mp/NetworkMessages.hpp>
#include <PacketPriority.h>
#include <RakPeer.h>
#include "ActorPacket.hpp"

using namespace mwmp;

ActorPacket::ActorPacket(RakNet::RakPeerInterface *peer) : BasePacket(peer)
{
    packetID = 0;
    priority = HIGH_PRIORITY;
    reliability = RELIABLE_ORDERED;
    orderChannel = CHANNEL_ACTOR;
    this->peer = peer;
}

ActorPacket::~ActorPacket()
{

}

void ActorPacket::setActorList(BaseActorList *actorList)
{
    this->actorList = actorList;
    guid = actorList->guid;
}
