//
// Created by koncord on 29.08.16.
//

#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerClass.hpp"

mwmp::PacketPlayerClass::PacketPlayerClass(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_CHARCLASS;
}

void mwmp::PacketPlayerClass::Packet(RakNet::BitStream *bs, mwmp::BasePlayer *player, bool send)
{
    PlayerPacket::Packet(bs, player, send);

    RW(player->charClass.mId, send);
    if (player->charClass.mId.empty()) // custom class
    {
        RW(player->charClass.mName, send);
        RW(player->charClass.mDescription, send);
        RW(player->charClass.mData, send);
    }
}
