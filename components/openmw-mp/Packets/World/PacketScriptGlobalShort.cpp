#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketScriptGlobalShort.hpp"

using namespace mwmp;

PacketScriptGlobalShort::PacketScriptGlobalShort(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_SCRIPT_GLOBAL_SHORT;
}

void PacketScriptGlobalShort::Object(WorldObject &worldObject, bool send)
{
    RW(worldObject.varName, send);
    RW(worldObject.shortVal, send);
}
