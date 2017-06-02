#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketObjectScale.hpp"

using namespace mwmp;

PacketObjectScale::PacketObjectScale(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_OBJECT_SCALE;
    hasCellData = true;
}

void PacketObjectScale::Object(WorldObject &worldObject, bool send)
{
    WorldPacket::Object(worldObject, send);
    RW(worldObject.scale, send);
}
