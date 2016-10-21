#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketObjectCreation.hpp"

using namespace mwmp;

PacketObjectCreation::PacketObjectCreation(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_WORLD_OBJECT_REMOVAL;
}

void PacketObjectCreation::Packet(RakNet::BitStream *bs, WorldEvent *event, bool send)
{
    WorldPacket::Packet(bs, event, send);

    RW(*event->CellRef(), send);
}
