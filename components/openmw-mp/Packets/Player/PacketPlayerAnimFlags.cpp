//
// Created by koncord on 15.01.16.
//

#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerAnimFlags.hpp"

mwmp::PacketPlayerAnimFlags::PacketPlayerAnimFlags(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_ANIM_FLAGS;
}

void mwmp::PacketPlayerAnimFlags::Packet(RakNet::BitStream *bs, bool send)
{
    PlayerPacket::Packet(bs, send);

    RW(player->movementFlags, send);
    RW(player->drawState, send);
    RW(player->isFlying, send);
}
