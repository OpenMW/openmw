#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketScriptGlobalShort.hpp"

using namespace mwmp;

PacketScriptGlobalShort::PacketScriptGlobalShort(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_SCRIPT_GLOBAL_SHORT;
}

void PacketScriptGlobalShort::Packet(RakNet::BitStream *bs, WorldEvent *event, bool send)
{
    WorldPacket::Packet(bs, event, send);

    RW(event->globalName, send);
    RW(event->shortVal, send);
}
