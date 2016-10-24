#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketObjectScale.hpp"

using namespace mwmp;

PacketObjectScale::PacketObjectScale(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_WORLD_OBJECT_SCALE;
}

void PacketObjectScale::Packet(RakNet::BitStream *bs, WorldEvent *event, bool send)
{
    WorldPacket::Packet(bs, event, send);

    RW(event->cellRef.mRefID, send);
    RW(event->cellRef.mRefNum.mIndex, send);

    RW(event->cell.mData.mFlags, send);
    RW(event->cell.mData.mX, send);
    RW(event->cell.mData.mY, send);
    RW(event->cell.mName, send);

    RW(event->scale, send);
}
