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

void PacketPlayerBaseInfo::Packet(RakNet::BitStream *bs, bool send)
{
    PlayerPacket::Packet(bs, send);

    RW(player->npc.mName, send, 1);
    RW(player->npc.mModel, send, 1);
    RW(player->npc.mRace, send, 1);
    RW(player->npc.mHair, send, 1);
    RW(player->npc.mHead, send, 1);

    RW(player->npc.mFlags, send);

    RW(player->birthsign, send, 1);

    RW(player->creatureModel, send, 1);
    RW(player->useCreatureName, send);
}
