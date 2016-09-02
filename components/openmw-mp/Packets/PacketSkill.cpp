//
// Created by koncord on 17.03.16.
//

#include "PacketSkill.hpp"

#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/esm/creaturestats.hpp>

using namespace mwmp;

PacketSkill::PacketSkill(RakNet::RakPeerInterface *peer) : BasePacket(peer)
{
    packetID = ID_GAME_SKILL;
}

void PacketSkill::Packet(RakNet::BitStream *bs, BasePlayer *player, bool send)
{
    BasePacket::Packet(bs, player, send);

    for (int i = 0; i < StatsCount; ++i)
        RW(player->NpcStats()->mSkills[i], send);
}
