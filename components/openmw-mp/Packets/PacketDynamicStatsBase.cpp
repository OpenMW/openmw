//
// Created by David Cernat on 28.09.16.
//

#include "PacketDynamicStatsBase.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketDynamicStatsBase::PacketDynamicStatsBase(RakNet::RakPeerInterface *peer) : BasePacket(peer)
{
    packetID = ID_GAME_DYNAMICSTATS_BASE;
}

void PacketDynamicStatsBase::Packet(RakNet::BitStream *bs, BasePlayer *player, bool send)
{
    BasePacket::Packet(bs, player, send);
    RW(player->CreatureStats()->mDynamic[0], send); // health
    RW(player->CreatureStats()->mDynamic[1], send); // magic
    RW(player->CreatureStats()->mDynamic[2], send); // fatigue
}