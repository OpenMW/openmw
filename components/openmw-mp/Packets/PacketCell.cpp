//
// Created by koncord on 15.01.16.
//

#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketCell.hpp"


mwmp::PacketCell::PacketCell(RakNet::RakPeerInterface *peer) : BasePacket(peer)
{
    packetID = ID_GAME_CELL;
    priority = IMMEDIATE_PRIORITY;
    reliability = RELIABLE_ORDERED;
}

void mwmp::PacketCell::Packet(RakNet::BitStream *bs, mwmp::BasePlayer *player, bool send)
{
    BasePacket::Packet(bs, player, send);

    RW(player->GetCell()->mData.mFlags, send);

    RW(player->GetCell()->mCellId.mIndex.mX, send);
    RW(player->GetCell()->mCellId.mIndex.mY, send);

    RW(player->GetCell()->mName, send);
}
