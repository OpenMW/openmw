//
// Created by koncord on 22.10.16.
//

#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketInventory.hpp"

using namespace std;
using namespace mwmp;

PacketInventory::PacketInventory(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_GAME_INVENTORY;
}

void PacketInventory::Packet(RakNet::BitStream *bs, BasePlayer *player, bool send)
{
    PlayerPacket::Packet(bs, player, send);

    RW(player->packetItems.action, send);

    if (!send)
        player->packetItems.items.clear();
    else
        player->packetItems.count = (unsigned int) (player->packetItems.items.size());

    RW(player->packetItems.count, send);

    for (unsigned int i = 0; i < player->packetItems.count; i++)
    {
        Item item;

        if (send)
        {
            item = player->packetItems.items[i];
            RW(item.refid, send);
            RW(item.count, send);
            RW(item.health, send);
        }
        else
        {
            RW(item.refid, send);
            RW(item.count, send);
            RW(item.health, send);
            player->packetItems.items.push_back(item);
        }
    }
}
