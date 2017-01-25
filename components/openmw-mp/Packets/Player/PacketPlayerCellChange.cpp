//
// Created by koncord on 15.01.16.
//

#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerCellChange.hpp"


mwmp::PacketPlayerCellChange::PacketPlayerCellChange(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_CELL_CHANGE;
    priority = IMMEDIATE_PRIORITY;
    reliability = RELIABLE_ORDERED;
}

void mwmp::PacketPlayerCellChange::Packet(RakNet::BitStream *bs, mwmp::BasePlayer *player, bool send)
{
    PlayerPacket::Packet(bs, player, send);

    RW(player->cell.mData.mFlags, send);
    RW(player->cell.mData.mX, send);
    RW(player->cell.mData.mY, send);
    RW(player->cell.mName, send);
}
