//
// Created by koncord on 02.03.16.
//

#include "Items.hpp"
#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <apps/openmw-mp/Networking.hpp>
#include <components/misc/stringops.hpp>

using namespace mwmp;

void ItemFunctions::addItem(unsigned short pid, const char* itemName, unsigned int count, int health) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    Item item;
    item.refid = itemName;
    item.count = count;
    item.health = health;

    player->inventorySendBuffer.items.push_back(item);
    player->inventorySendBuffer.action = Inventory::ADDITEM;
}

void ItemFunctions::equipItem(unsigned short pid, unsigned short slot, const char *itemName, unsigned int count, int health) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->EquipedItem(slot)->refid = itemName;
    player->EquipedItem(slot)->count = count;
    player->EquipedItem(slot)->health = health;
}

void ItemFunctions::unequipItem(unsigned short pid, unsigned short slot) noexcept
{
    LOG_MESSAGE(Log::LOG_WARN, "%s", "stub");
    //ItemFunctions::equipItem(pid, slot, "", 0);
}

const char *ItemFunctions::getItemSlot(unsigned short pid, unsigned short slot) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->EquipedItem(slot)->refid.c_str();
}

bool ItemFunctions::hasItemEquipped(unsigned short pid, const char* itemName)
{
    Player *player;
    GET_PLAYER(pid, player, false);

    for (int slot = 0; slot < 27; slot ++)
        if (Misc::StringUtils::ciEqual(player->EquipedItem(slot)->refid, itemName))
            return true;
    return false;
}

void ItemFunctions::removeItem(unsigned short pid, const char* itemName, unsigned short count) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    Item item;
    item.refid = itemName;
    item.count = count;

    player->inventorySendBuffer.items.clear();
    player->inventorySendBuffer.items.push_back(item);
    player->inventorySendBuffer.action = Inventory::REMOVEITEM;
}
void ItemFunctions::getItemCount(unsigned short pid, const char* itemName) noexcept
{
    LOG_MESSAGE(Log::LOG_WARN, "%s", "stub");
}

const char *ItemFunctions::getItemName(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, "");

    return player->inventory.items.at(i).refid.c_str();
}

int ItemFunctions::getItemCount2(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->inventory.items.at(i).count;
}

int ItemFunctions::getItemHealth(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->inventory.items.at(i).health;
}

unsigned int ItemFunctions::getInventorySize(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->inventory.count;
}

void ItemFunctions::sendEquipment(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::Networking::get().getPlayerController()->GetPacket(ID_GAME_EQUIPMENT)->Send(player, false);
    mwmp::Networking::get().getPlayerController()->GetPacket(ID_GAME_EQUIPMENT)->Send(player, true);
}

void ItemFunctions::sendInventory(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );
    std::swap(player->inventory, player->inventorySendBuffer);
    mwmp::Networking::get().getPlayerController()->GetPacket(ID_GAME_INVENTORY)->Send(player, false);
    player->inventory = std::move(player->inventorySendBuffer);
    player->inventorySendBuffer.items.clear();
}
