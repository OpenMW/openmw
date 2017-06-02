#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketDoorState.hpp"

using namespace mwmp;

PacketDoorState::PacketDoorState(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_DOOR_STATE;
    hasCellData = true;
}

void PacketDoorState::Object(WorldObject &worldObject, bool send)
{
    WorldPacket::Object(worldObject, send);
    RW(worldObject.doorState, send);
}
