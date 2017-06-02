#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketObjectPlace.hpp"

using namespace mwmp;

PacketObjectPlace::PacketObjectPlace(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_OBJECT_PLACE;
    hasCellData = true;
}

void PacketObjectPlace::Object(WorldObject &worldObject, bool send)
{
    WorldPacket::Object(worldObject, send);
    RW(worldObject.count, send);
    RW(worldObject.charge, send);
    RW(worldObject.goldValue, send);
    RW(worldObject.position, send);
}
