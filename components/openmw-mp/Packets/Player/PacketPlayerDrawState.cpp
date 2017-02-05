//
// Created by koncord on 15.01.16.
//

#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerDrawState.hpp"

mwmp::PacketPlayerDrawState::PacketPlayerDrawState(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_DRAWSTATE;
}

void mwmp::PacketPlayerDrawState::Packet(RakNet::BitStream *bs, mwmp::BasePlayer *player, bool send)
{
    PlayerPacket::Packet(bs, player, send);

    RW(player->movementFlags, send);

    RW(player->drawState, send);
}
