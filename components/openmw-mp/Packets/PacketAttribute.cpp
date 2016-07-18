//
// Created by koncord on 08.03.16.
//

#include "PacketAttribute.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketAttribute::PacketAttribute(RakNet::RakPeerInterface *peer) : BasePacket(peer)
{
    packetID = ID_GAME_ATTRIBUTE;
}

void PacketAttribute::Packet(RakNet::BitStream *bs, BasePlayer *player, bool send)
{
    BasePacket::Packet(bs, player, send);

    for(int i = 0; i < AttributesCount; ++i)
        RW(player->CreatureStats()->mAttributes[i], send);
}
