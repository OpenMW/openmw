#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketContainerRemove.hpp"

using namespace mwmp;

PacketContainerRemove::PacketContainerRemove(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_WORLD_CONTAINER_REMOVE;
}

void PacketContainerRemove::Packet(RakNet::BitStream *bs, WorldEvent *event, bool send)
{
    WorldPacket::Packet(bs, event, send);
}
