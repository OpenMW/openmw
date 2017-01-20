//
// Created by koncord on 02.03.16.
//

#include "Items.hpp"
#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <apps/openmw-mp/Networking.hpp>
#include <apps/openmw/mwworld/inventorystore.hpp>
#include <components/misc/stringops.hpp>

using namespace mwmp;

int ItemFunctions::GetEquipmentSize() noexcept
{
    return MWWorld::InventoryStore::Slots;
}

unsigned int ItemFunctions::GetInventorySize(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->packetItems.count;
}

void ItemFunctions::EquipItem(unsigned short pid, unsigned short slot, const char *itemId, unsigned int count, int health) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->EquipedItem(slot)->refid = itemId;
    player->EquipedItem(slot)->count = count;
    player->EquipedItem(slot)->health = health;
}

void ItemFunctions::UnequipItem(unsigned short pid, unsigned short slot) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    ItemFunctions::EquipItem(pid, slot, "", 0, -1);
}

void ItemFunctions::AddItem(unsigned short pid, const char* itemId, unsigned int count, int health) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    Item item;
    item.refid = itemId;
    item.count = count;
    item.health = health;

    player->packetItemsBuffer.items.push_back(item);
    player->packetItemsBuffer.action = PacketItems::ADD;
}

void ItemFunctions::RemoveItem(unsigned short pid, const char* itemId, unsigned short count) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    Item item;
    item.refid = itemId;
    item.count = count;

    player->packetItemsBuffer.items.push_back(item);
    player->packetItemsBuffer.action = PacketItems::REMOVE;
}

void ItemFunctions::ClearInventory(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->packetItemsBuffer.items.clear();
    player->packetItemsBuffer.action = PacketItems::SET;
}

bool ItemFunctions::HasItemEquipped(unsigned short pid, const char* itemId)
{
    Player *player;
    GET_PLAYER(pid, player, false);

    for (int slot = 0; slot < MWWorld::InventoryStore::Slots; slot++)
        if (Misc::StringUtils::ciEqual(player->EquipedItem(slot)->refid, itemId))
            return true;
    return false;
}

const char *ItemFunctions::GetEquipmentItemId(unsigned short pid, unsigned short slot) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->EquipedItem(slot)->refid.c_str();
}

int ItemFunctions::GetEquipmentItemCount(unsigned short pid, unsigned short slot) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->EquipedItem(slot)->count;
}

int ItemFunctions::GetEquipmentItemHealth(unsigned short pid, unsigned short slot) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->EquipedItem(slot)->health;
}

const char *ItemFunctions::GetInventoryItemId(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, "");

    if (i >= player->packetItems.count)
        return "invalid";

    return player->packetItems.items.at(i).refid.c_str();
}

int ItemFunctions::GetInventoryItemCount(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->packetItems.items.at(i).count;
}

int ItemFunctions::GetInventoryItemHealth(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->packetItems.items.at(i).health;
}

void ItemFunctions::SendEquipment(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::Networking::get().getPlayerController()->GetPacket(ID_GAME_EQUIPMENT)->Send(player, false);
    mwmp::Networking::get().getPlayerController()->GetPacket(ID_GAME_EQUIPMENT)->Send(player, true);
}

void ItemFunctions::SendItems(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    std::swap(player->packetItems, player->packetItemsBuffer);
    mwmp::Networking::get().getPlayerController()->GetPacket(ID_GAME_INVENTORY)->Send(player, false);
    player->packetItems = std::move(player->packetItemsBuffer);
    player->packetItemsBuffer.items.clear();
}
