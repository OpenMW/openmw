#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketObjectPlace.hpp"

using namespace mwmp;

PacketObjectPlace::PacketObjectPlace(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_WORLD_OBJECT_DELETE;
}

void PacketObjectPlace::Packet(RakNet::BitStream *bs, WorldEvent *event, bool send)
{
    WorldPacket::Packet(bs, event, send);

    RW(event->cellRef.mRefID, send);
    RW(event->cellRef.mRefNum, send);
}
