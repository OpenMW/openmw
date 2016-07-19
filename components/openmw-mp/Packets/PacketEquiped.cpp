//
// Created by koncord on 07.01.16.
//

#include "PacketEquiped.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketEquiped::PacketEquiped(RakNet::RakPeerInterface *peer) : BasePacket(peer)
{
    packetID = ID_GAME_UPDATE_EQUIPED;
}

void PacketEquiped::Packet(RakNet::BitStream *bs, BasePlayer *player, bool send)
{
    BasePacket::Packet(bs, player, send);

    for(int i = 0; i < 19; i++)
    {
        RW(player->EquipedItem(i)->refid, send);
        RW(player->EquipedItem(i)->count, send);
    }
}
