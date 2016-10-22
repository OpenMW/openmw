#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketObjectDelete.hpp"

using namespace mwmp;

PacketObjectDelete::PacketObjectDelete(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_WORLD_OBJECT_DELETE;
}

void PacketObjectDelete::Packet(RakNet::BitStream *bs, WorldEvent *event, bool send)
{
    WorldPacket::Packet(bs, event, send);

    RW(event->cellRef.mRefID, send);
    RW(event->cellRef.mRefNum, send);

    RW(event->cellId.mPaged, send);
    RW(event->cellId.mIndex.mX, send);
    RW(event->cellId.mIndex.mY, send);
    RW(event->cellId.mWorldspace, send);
}
