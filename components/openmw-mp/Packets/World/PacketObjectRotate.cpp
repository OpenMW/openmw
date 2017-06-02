#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketObjectRotate.hpp"

using namespace mwmp;

PacketObjectRotate::PacketObjectRotate(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_OBJECT_ROTATE;
    hasCellData = true;
}

void PacketObjectRotate::Object(WorldObject &worldObject, bool send)
{
    WorldPacket::Object(worldObject, send);
    RW(worldObject.position.rot[0], send);
    RW(worldObject.position.rot[1], send);
    RW(worldObject.position.rot[2], send);
}
