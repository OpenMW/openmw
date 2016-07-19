//
// Created by koncord on 15.01.16.
//

#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketDrawState.hpp"

mwmp::PacketDrawState::PacketDrawState(RakNet::RakPeerInterface *peer) : BasePacket(peer)
{
    packetID = ID_GAME_DRAWSTATE;
}

void mwmp::PacketDrawState::Packet(RakNet::BitStream *bs, mwmp::BasePlayer *player, bool send)
{
    BasePacket::Packet(bs, player, send);

    RW(*player->MovementFlags(), send);

    RW(*player->DrawState(), send);
}
