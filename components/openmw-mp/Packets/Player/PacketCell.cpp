//
// Created by koncord on 15.01.16.
//

#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketCell.hpp"


mwmp::PacketCell::PacketCell(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_GAME_CELL;
    priority = IMMEDIATE_PRIORITY;
    reliability = RELIABLE_ORDERED;
}

void mwmp::PacketCell::Packet(RakNet::BitStream *bs, mwmp::BasePlayer *player, bool send)
{
    PlayerPacket::Packet(bs, player, send);

    RW(player->getCell()->mData.mFlags, send);
    RW(player->getCell()->mData.mX, send);
    RW(player->getCell()->mData.mY, send);
    RW(player->getCell()->mName, send);
}
