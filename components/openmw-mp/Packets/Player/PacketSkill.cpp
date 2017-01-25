//
// Created by koncord on 17.03.16.
//

#include "PacketSkill.hpp"

#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/esm/creaturestats.hpp>

using namespace mwmp;

PacketSkill::PacketSkill(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_GAME_SKILL;
}

void PacketSkill::Packet(RakNet::BitStream *bs, BasePlayer *player, bool send)
{
    PlayerPacket::Packet(bs, player, send);

    for (int i = 0; i < SkillCount; ++i)
        RW(player->npcStats.mSkills[i], send);

    for (int i = 0; i < AttributeCount; ++i)
        RW(player->npcStats.mSkillIncrease[i], send);

    RW(player->npcStats.mLevelProgress, send);
}
