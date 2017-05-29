//
// Created by koncord on 30.08.16.
//

#include "PacketGameTime.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketGameTime::PacketGameTime(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_GAME_TIME;
    orderChannel = CHANNEL_SYSTEM;
}

void PacketGameTime::Packet(RakNet::BitStream *bs, bool send)
{
    PlayerPacket::Packet(bs, send);

    RW(player->month, send);
    RW(player->day, send);
    RW(player->hour, send);
}
