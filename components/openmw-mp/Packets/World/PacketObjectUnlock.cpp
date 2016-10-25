#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketObjectUnlock.hpp"

using namespace mwmp;

PacketObjectUnlock::PacketObjectUnlock(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_OBJECT_UNLOCK;
}

void PacketObjectUnlock::Packet(RakNet::BitStream *bs, WorldEvent *event, bool send)
{
    WorldPacket::Packet(bs, event, send);

    RW(event->cellRef.mRefID, send);
    RW(event->cellRef.mRefNum.mIndex, send);

    RW(event->cell.mData.mFlags, send);
    RW(event->cell.mData.mX, send);
    RW(event->cell.mData.mY, send);
    RW(event->cell.mName, send);
}
