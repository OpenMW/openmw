//
// Created by koncord on 11.01.16.
//

#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/esm/creaturestats.hpp>
#include "PacketAttributesAndStats.hpp"

using namespace mwmp;

PacketAttributesAndStats::PacketAttributesAndStats(RakNet::RakPeerInterface *peer) : BasePacket(peer)
{
    packetID = ID_GAME_UPDATE_SKILLS;
}

void PacketAttributesAndStats::Packet(RakNet::BitStream *bs, BasePlayer *player, bool send)
{
    BasePacket::Packet(bs, player, send);

    for (int i = 0; i < AttributesCount; ++i)
        RW(player->CreatureStats()->mAttributes[i], send);

    for (int i = 0; i < StatsCount; ++i)
        RW(player->NpcStats()->mSkills[i], send);
}
