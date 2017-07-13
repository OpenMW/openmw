#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketObjectState.hpp"

using namespace mwmp;

PacketObjectState::PacketObjectState(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_OBJECT_STATE;
    hasCellData = true;
}

void PacketObjectState::Object(WorldObject &worldObject, bool send)
{
    WorldPacket::Object(worldObject, send);
    RW(worldObject.objectState, send);
}
