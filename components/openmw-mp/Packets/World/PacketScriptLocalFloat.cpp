#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketScriptLocalFloat.hpp"

using namespace mwmp;

PacketScriptLocalFloat::PacketScriptLocalFloat(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_SCRIPT_LOCAL_FLOAT;
}

void PacketScriptLocalFloat::Packet(RakNet::BitStream *bs, WorldEvent *event, bool send)
{
    WorldPacket::Packet(bs, event, send);

    RW(event->cellRef.mRefID, send);
    RW(event->cellRef.mRefNum.mIndex, send);

    RW(event->cell.mData.mFlags, send);
    RW(event->cell.mData.mX, send);
    RW(event->cell.mData.mY, send);
    RW(event->cell.mName, send);

    RW(event->index, send);
    RW(event->floatVal, send);
}
