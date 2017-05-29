#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerRegionAuthority.hpp"

mwmp::PacketPlayerRegionAuthority::PacketPlayerRegionAuthority(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_REGION_AUTHORITY;
    priority = IMMEDIATE_PRIORITY;
    reliability = RELIABLE_ORDERED;
}

void mwmp::PacketPlayerRegionAuthority::Packet(RakNet::BitStream *bs, bool send)
{
    PlayerPacket::Packet(bs, send);

    // Placeholder to be filled in later
}
