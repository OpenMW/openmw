#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketObjectMove.hpp"

using namespace mwmp;

PacketObjectMove::PacketObjectMove(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_OBJECT_MOVE;
}

void PacketObjectMove::Packet(RakNet::BitStream *bs, WorldEvent *event, bool send)
{
    WorldPacket::Packet(bs, event, send);
}
