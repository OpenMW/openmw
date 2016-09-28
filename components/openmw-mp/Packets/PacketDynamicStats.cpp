//
// Created by David Cernat on 28.09.16.
//

#include "PacketDynamicStats.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketDynamicStats::PacketDynamicStats(RakNet::RakPeerInterface *peer) : BasePacket(peer)
{
    packetID = ID_GAME_DYNAMICSTATS;
}

void PacketDynamicStats::Packet(RakNet::BitStream *bs, BasePlayer *player, bool send)
{
    BasePacket::Packet(bs, player, send);
    RW(player->CreatureStats()->mDynamic[0], send); // health
    RW(player->CreatureStats()->mDynamic[1], send); // magic
    RW(player->CreatureStats()->mDynamic[2], send); // fatigue
}