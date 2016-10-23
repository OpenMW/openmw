#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketObjectPlace.hpp"

using namespace mwmp;

PacketObjectPlace::PacketObjectPlace(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_WORLD_OBJECT_PLACE;
}

void PacketObjectPlace::Packet(RakNet::BitStream *bs, WorldEvent *event, bool send)
{
    WorldPacket::Packet(bs, event, send);

    RW(event->cellRef.mRefID, send);
    RW(event->cellRef.mRefNum.mIndex, send);
    RW(event->cellRef.mPos, send);

    RW(event->cell.mData.mFlags, send);
    RW(event->cell.mCellId.mIndex.mX, send);
    RW(event->cell.mCellId.mIndex.mY, send);
    RW(event->cell.mName, send);
}
