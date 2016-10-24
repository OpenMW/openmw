#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketContainerAdd.hpp"

using namespace mwmp;

PacketContainerAdd::PacketContainerAdd(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_WORLD_CONTAINER_ADD;
}

void PacketContainerAdd::Packet(RakNet::BitStream *bs, WorldEvent *event, bool send)
{
    WorldPacket::Packet(bs, event, send);
}
