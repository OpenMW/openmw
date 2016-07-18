//
// Created by koncord on 13.01.16.
//

#include "PacketMainStats.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketMainStats::PacketMainStats(RakNet::RakPeerInterface *peer) : BasePacket(peer)
{
    packetID = ID_GAME_UPDATE_BASESTATS;
}

void PacketMainStats::Packet(RakNet::BitStream *bs, BasePlayer *player, bool send)
{
    BasePacket::Packet(bs, player, send);
    RW(player->CreatureStats()->mDynamic[0], send); // health
    RW(player->CreatureStats()->mDynamic[1], send); // magic
    RW(player->CreatureStats()->mDynamic[2], send); // fatigue
}