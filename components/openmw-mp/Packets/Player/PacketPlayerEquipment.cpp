//
// Created by koncord on 07.01.16.
//

#include "PacketPlayerEquipment.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketPlayerEquipment::PacketPlayerEquipment(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_EQUIPMENT;
}

void PacketPlayerEquipment::Packet(RakNet::BitStream *bs, BasePlayer *player, bool send)
{
    PlayerPacket::Packet(bs, player, send);

    for (int i = 0; i < 19; i++)
    {
        RW(player->equipedItems[i].refId, send);
        RW(player->equipedItems[i].count, send);
        RW(player->equipedItems[i].charge, send);
    }
}
