//
// Created by koncord on 29.08.16.
//

#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketClass.hpp"

mwmp::PacketClass::PacketClass(RakNet::RakPeerInterface *peer) : BasePacket(peer)
{
    packetID = ID_GAME_CHARCLASS;
}

void mwmp::PacketClass::Packet(RakNet::BitStream *bs, mwmp::BasePlayer *player, bool send)
{
    BasePacket::Packet(bs, player, send);

    RW(player->charClass.mId, send);
    if (player->charClass.mId.empty()) // custom class
    {
        RW(player->charClass.mName, send);
        RW(player->charClass.mDescription, send);
        RW(player->charClass.mData, send);
    }
}
