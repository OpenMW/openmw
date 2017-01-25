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

    RW(player->npc.mName, send);
    RW(player->npc.mModel, send);
    RW(player->npc.mRace, send);
    RW(player->npc.mHair, send);
    RW(player->npc.mHead, send);

    RW(player->npc.mFlags, send);

    RW(player->birthsign, send);
}
