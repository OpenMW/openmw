//
// Created by David Cernat on 25.09.16.
//

#include "PacketLevel.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketLevel::PacketLevel(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_GAME_LEVEL;
}

void PacketLevel::Packet(RakNet::BitStream *bs, BasePlayer *player, bool send)
{
    PlayerPacket::Packet(bs, player, send);

    RW(player->creatureStats.mLevel, send);
}
