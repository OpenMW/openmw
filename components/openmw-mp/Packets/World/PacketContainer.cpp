#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketContainer.hpp"

using namespace mwmp;

PacketContainer::PacketContainer(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_CONTAINER;
}

void PacketContainer::Packet(RakNet::BitStream *bs, WorldEvent *event, bool send)
{
    WorldPacket::Packet(bs, event, send);
}
