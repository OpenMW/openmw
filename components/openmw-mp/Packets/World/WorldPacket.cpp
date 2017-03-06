#include <components/openmw-mp/NetworkMessages.hpp>
#include <PacketPriority.h>
#include <RakPeer.h>
#include "WorldPacket.hpp"

using namespace mwmp;

WorldPacket::WorldPacket(RakNet::RakPeerInterface *peer) : BasePacket(peer)
{
    packetID = 0;
    priority = HIGH_PRIORITY;
    reliability = RELIABLE_ORDERED;
    this->peer = peer;
}

WorldPacket::~WorldPacket()
{

}

void WorldPacket::setEvent(BaseEvent *event)
{
    this->event = event;
    guid = event->guid;
}
