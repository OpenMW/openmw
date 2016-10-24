#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketActivatorActivate.hpp"

using namespace mwmp;

PacketActivatorActivate::PacketActivatorActivate(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_WORLD_ACTIVATOR_ACTIVATE;
}

void PacketActivatorActivate::Packet(RakNet::BitStream *bs, WorldEvent *event, bool send)
{
    WorldPacket::Packet(bs, event, send);
}
