#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketScriptMemberShort.hpp"

using namespace mwmp;

PacketScriptMemberShort::PacketScriptMemberShort(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_SCRIPT_MEMBER_SHORT;
}

void PacketScriptMemberShort::Object(WorldObject &worldObject, bool send)
{
    RW(worldObject.refId, send);
    RW(worldObject.index, send);
    RW(worldObject.shortVal, send);
}
