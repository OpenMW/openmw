#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketDoorActivate.hpp"

using namespace mwmp;

PacketDoorActivate::PacketDoorActivate(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_DOOR_ACTIVATE;
}

void PacketDoorActivate::Packet(RakNet::BitStream *bs, WorldEvent *event, bool send)
{
    WorldPacket::Packet(bs, event, send);

    RW(event->cellRef.mRefID, send);
    RW(event->cellRef.mRefNum.mIndex, send);

    RW(event->cell.mData.mFlags, send);
    RW(event->cell.mData.mX, send);
    RW(event->cell.mData.mY, send);
    RW(event->cell.mName, send);

    RW(event->state, send);
}
