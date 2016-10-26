#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketScriptMemberShort.hpp"

using namespace mwmp;

PacketScriptMemberShort::PacketScriptMemberShort(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_SCRIPT_MEMBER_SHORT;
}

void PacketScriptMemberShort::Packet(RakNet::BitStream *bs, WorldEvent *event, bool send)
{
    WorldPacket::Packet(bs, event, send);

    RW(event->cellRef.mRefID, send);

    RW(event->index, send);
    RW(event->shortVal, send);
}
