#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketObjectRemoval.hpp"

using namespace mwmp;

PacketObjectRemoval::PacketObjectRemoval(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_WORLD_OBJECT_REMOVAL;
}

void PacketObjectRemoval::Packet(RakNet::BitStream *bs, WorldEvent *event, bool send)
{
    WorldPacket::Packet(bs, event, send);

    RW(*event->CellRef(), send);
}
