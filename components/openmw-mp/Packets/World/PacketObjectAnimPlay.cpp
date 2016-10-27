#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketObjectAnimPlay.hpp"

using namespace mwmp;

PacketObjectAnimPlay::PacketObjectAnimPlay(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_OBJECT_ANIM_PLAY;
}

void PacketObjectAnimPlay::Packet(RakNet::BitStream *bs, WorldEvent *event, bool send)
{
    WorldPacket::Packet(bs, event, send);

    RW(event->cellRef.mRefID, send);
    RW(event->cellRef.mRefNum.mIndex, send);

    RW(event->cell.mData.mFlags, send);
    RW(event->cell.mData.mX, send);
    RW(event->cell.mData.mY, send);
    RW(event->cell.mName, send);

    RW(event->animGroup, send);
    RW(event->animMode, send);
}
