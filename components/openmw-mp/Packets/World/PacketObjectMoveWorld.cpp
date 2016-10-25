#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketObjectMoveWorld.hpp"

using namespace mwmp;

PacketObjectMoveWorld::PacketObjectMoveWorld(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_OBJECT_MOVE_WORLD;
}

void PacketObjectMoveWorld::Packet(RakNet::BitStream *bs, WorldEvent *event, bool send)
{
    WorldPacket::Packet(bs, event, send);
}
