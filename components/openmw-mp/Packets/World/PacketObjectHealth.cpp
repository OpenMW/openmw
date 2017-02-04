#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketObjectHealth.hpp"

using namespace mwmp;

PacketObjectHealth::PacketObjectHealth(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_OBJECT_HEALTH;
}

void PacketObjectHealth::Packet(RakNet::BitStream *bs, WorldEvent *event, bool send)
{
    WorldPacket::Packet(bs, event, send);
}
