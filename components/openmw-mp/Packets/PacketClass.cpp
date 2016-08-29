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

    RW(player->klass.mId, send);
    RW(player->customClass, send);

    if(player->customClass)
    {
        RW(player->klass.mName, send);
        RW(player->klass.mDescription, send);
        RW(player->klass.mData, send);
    }
}
