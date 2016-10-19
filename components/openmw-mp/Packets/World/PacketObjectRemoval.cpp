#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketObjectRemoval.hpp"

using namespace mwmp;

PacketObjectRemoval::PacketObjectRemoval(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_WORLD_OBJECT_REMOVAL;
}

void PacketObjectRemoval::Packet(RakNet::BitStream *bs, BasePlayer *player, bool send)
{
    WorldPacket::Packet(bs, player, send);
}
