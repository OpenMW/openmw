//
// Created by koncord on 07.01.16.
//

#include "PacketBaseInfo.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketBaseInfo::PacketBaseInfo(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_GAME_BASE_INFO;
}

void PacketBaseInfo::Packet(RakNet::BitStream *bs, BasePlayer *player, bool send)
{
    PlayerPacket::Packet(bs, player, send);

    RW(player->Npc()->mName, send);
    RW(player->Npc()->mModel, send);
    RW(player->Npc()->mRace, send);
    RW(player->Npc()->mHair, send);
    RW(player->Npc()->mHead, send);

    RW(player->Npc()->mFlags, send);

    RW(*player->BirthSign(), send);
}
