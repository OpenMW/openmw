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

    RW(player->inventory.action, send);

    if (!send)
        player->inventory.items.clear();
    else
        player->inventory.count = (unsigned int) (player->inventory.items.size());

    RW(player->inventory.count, send);

    for (int i = 0; i < player->inventory.count; i++)
    {
        Item item;

        if (send)
        {
            item = player->inventory.items[i];
            RW(item.refid, send);
            RW(item.count, send);
            RW(item.health, send);
        }
        else
        {
            RW(item.refid, send);
            RW(item.count, send);
            RW(item.health, send);
            player->inventory.items.push_back(item);
        }
    }
}
