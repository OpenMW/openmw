//
// Created by koncord on 07.01.16.
//

#include "PacketEquipment.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketEquipment::PacketEquipment(RakNet::RakPeerInterface *peer) : BasePacket(peer)
{
    packetID = ID_GAME_EQUIPMENT;
}

void PacketEquipment::Packet(RakNet::BitStream *bs, BasePlayer *player, bool send)
{
    BasePacket::Packet(bs, player, send);

    for (int i = 0; i < 19; i++)
    {
        RW(player->EquipedItem(i)->refid, send);
        RW(player->EquipedItem(i)->count, send);
    }
}
