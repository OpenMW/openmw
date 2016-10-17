//
// Created by koncord on 08.03.16.
//

#include "PacketAttribute.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketAttribute::PacketAttribute(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_GAME_ATTRIBUTE;
}

void PacketAttribute::Packet(RakNet::BitStream *bs, BasePlayer *player, bool send)
{
    PlayerPacket::Packet(bs, player, send);

    for (int i = 0; i < AttributeCount; ++i)
        RW(player->CreatureStats()->mAttributes[i], send);
}
