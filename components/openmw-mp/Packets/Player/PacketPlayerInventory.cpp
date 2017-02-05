//
// Created by koncord on 22.10.16.
//

#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerInventory.hpp"

using namespace std;
using namespace mwmp;

PacketPlayerInventory::PacketPlayerInventory(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_INVENTORY;
}

void PacketPlayerInventory::Packet(RakNet::BitStream *bs, BasePlayer *player, bool send)
{
    PlayerPacket::Packet(bs, player, send);

    RW(player->inventoryChanges.action, send);

    if (!send)
        player->inventoryChanges.items.clear();
    else
        player->inventoryChanges.count = (unsigned int) (player->inventoryChanges.items.size());

    RW(player->inventoryChanges.count, send);

    for (unsigned int i = 0; i < player->inventoryChanges.count; i++)
    {
        Item item;

        if (send)
        {
            item = player->inventoryChanges.items.at(i);
        }

        RW(item.refId, send);
        RW(item.count, send);
        RW(item.charge, send);

        if (!send)
        {
            player->inventoryChanges.items.push_back(item);
        }
    }
}
