//
// Created by koncord on 07.01.16.
//

#include "PacketPlayerBaseInfo.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketPlayerBaseInfo::PacketPlayerBaseInfo(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_BASEINFO;
}

void PacketPlayerBaseInfo::Packet(RakNet::BitStream *bs, BasePlayer *player, bool send)
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
