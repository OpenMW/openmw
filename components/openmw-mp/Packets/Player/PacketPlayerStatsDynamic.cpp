//
// Created by koncord on 13.01.16.
//

#include "PacketPlayerStatsDynamic.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketPlayerStatsDynamic::PacketPlayerStatsDynamic(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_STATS_DYNAMIC;
}

void PacketPlayerStatsDynamic::Packet(RakNet::BitStream *bs, bool send)
{
    PlayerPacket::Packet(bs, send);
    RW(player->creatureStats.mDynamic[0], send); // health
    RW(player->creatureStats.mDynamic[1], send); // magic
    RW(player->creatureStats.mDynamic[2], send); // fatigue
}
