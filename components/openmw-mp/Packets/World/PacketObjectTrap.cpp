#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketObjectTrap.hpp"

using namespace mwmp;

PacketObjectTrap::PacketObjectTrap(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_OBJECT_TRAP;
    hasCellData = true;
}

void PacketObjectTrap::Object(WorldObject &worldObject, bool send)
{
    WorldPacket::Object(worldObject, send);
    RW(worldObject.isDisarmed, send);

    if (!worldObject.isDisarmed)
        RW(worldObject.position, send);
}
