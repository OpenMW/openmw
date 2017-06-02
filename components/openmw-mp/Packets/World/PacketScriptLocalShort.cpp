#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketScriptLocalShort.hpp"

using namespace mwmp;

PacketScriptLocalShort::PacketScriptLocalShort(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_SCRIPT_LOCAL_SHORT;
    hasCellData = true;
}

void PacketScriptLocalShort::Object(WorldObject &worldObject, bool send)
{
    WorldPacket::Object(worldObject, send);
    RW(worldObject.index, send);
    RW(worldObject.shortVal, send);
}
