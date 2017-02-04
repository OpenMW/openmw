//
// Created by koncord on 08.03.16.
//

#include "PacketPlayerAttribute.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketPlayerAttribute::PacketPlayerAttribute(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_ATTRIBUTE;
}

void PacketPlayerAttribute::Packet(RakNet::BitStream *bs, BasePlayer *player, bool send)
{
    PlayerPacket::Packet(bs, player, send);

    for (int i = 0; i < AttributeCount; ++i)
        RW(player->creatureStats.mAttributes[i], send);
}
