//
// Created by koncord on 30.08.16.
//

#include "PacketTime.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketTime::PacketTime(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_GAME_TIME;
}

void PacketTime::Packet(RakNet::BitStream *bs, BasePlayer *player, bool send)
{
    PlayerPacket::Packet(bs, player, send);

    RW(player->month, send);
    RW(player->day, send);
    RW(player->hour, send);
}
