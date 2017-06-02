#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketObjectSpawn.hpp"

using namespace mwmp;

PacketObjectSpawn::PacketObjectSpawn(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_OBJECT_SPAWN;
    hasCellData = true;
}

void PacketObjectSpawn::Object(WorldObject &worldObject, bool send)
{
    WorldPacket::Object(worldObject, send);
    RW(worldObject.position, send);

    RW(worldObject.hasMaster, send);

    if (worldObject.hasMaster)
    {
        RW(worldObject.master.refNumIndex, send);
        RW(worldObject.master.mpNum, send);
        RW(worldObject.master.guid, send);
    }
}
