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

    RW(event->cellRef.mRefID, send);
    RW(event->cellRef.mRefNum.mIndex, send);

    RW(event->cell.mData.mFlags, send);
    RW(event->cell.mData.mX, send);
    RW(event->cell.mData.mY, send);
    RW(event->cell.mName, send);

    RW(event->pos.pos[0], send);
    RW(event->pos.pos[1], send);
    RW(event->pos.pos[2], send);
}
