//
// Created by koncord on 29.08.16.
//

#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerClass.hpp"

mwmp::PacketPlayerClass::PacketPlayerClass(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_CHARCLASS;
}

void mwmp::PacketPlayerClass::Packet(RakNet::BitStream *bs, bool send)
{
    PlayerPacket::Packet(bs,  send);

    RW(player->charClass.mId, send);
    if (player->charClass.mId.empty()) // custom class
    {
        RW(player->charClass.mName, send, 1);
        RW(player->charClass.mDescription, send, 1);
        RW(player->charClass.mData, send, 1);
    }
}
