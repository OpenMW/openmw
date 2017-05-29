#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerRegionChange.hpp"

mwmp::PacketPlayerRegionChange::PacketPlayerRegionChange(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_REGION_CHANGE;
    priority = IMMEDIATE_PRIORITY;
    reliability = RELIABLE_ORDERED;
}

void mwmp::PacketPlayerRegionChange::Packet(RakNet::BitStream *bs, bool send)
{
    PlayerPacket::Packet(bs, send);

    // Placeholder to be filled in later
}
