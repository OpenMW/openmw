#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketDoorActivate.hpp"

using namespace mwmp;

PacketDoorActivate::PacketDoorActivate(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_WORLD_DOOR_ACTIVATE;
}

void PacketDoorActivate::Packet(RakNet::BitStream *bs, WorldEvent *event, bool send)
{
    WorldPacket::Packet(bs, event, send);
}
