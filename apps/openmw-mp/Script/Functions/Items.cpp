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

void ItemFunctions::InitializeInventoryChanges(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->inventoryChanges.items.clear();
    player->inventoryChanges.action = InventoryChanges::SET;
}

int ItemFunctions::GetEquipmentSize() noexcept
{
    return MWWorld::InventoryStore::Slots;
}

unsigned int ItemFunctions::GetInventoryChangesSize(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->inventoryChanges.count;
}

void ItemFunctions::EquipItem(unsigned short pid, unsigned short slot, const char *refId, unsigned int count, int charge) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->equipedItems[slot].refId = refId;
    player->equipedItems[slot].count = count;
    player->equipedItems[slot].charge = charge;
}

void ItemFunctions::UnequipItem(unsigned short pid, unsigned short slot) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    ItemFunctions::EquipItem(pid, slot, "", 0, -1);
}

void ItemFunctions::AddItem(unsigned short pid, const char* refId, unsigned int count, int charge) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    Item item;
    item.refId = refId;
    item.count = count;
    item.charge = charge;

    player->inventoryChanges.items.push_back(item);
    player->inventoryChanges.action = InventoryChanges::ADD;
}

void ItemFunctions::RemoveItem(unsigned short pid, const char* refId, unsigned short count) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    Item item;
    item.refId = refId;
    item.count = count;

    player->inventoryChanges.items.push_back(item);
    player->inventoryChanges.action = InventoryChanges::REMOVE;
}

bool ItemFunctions::HasItemEquipped(unsigned short pid, const char* refId)
{
    Player *player;
    GET_PLAYER(pid, player, false);

    for (int slot = 0; slot < MWWorld::InventoryStore::Slots; slot++)
        if (Misc::StringUtils::ciEqual(player->equipedItems[slot].refId, refId))
            return true;
    return false;
}

const char *ItemFunctions::GetEquipmentItemRefId(unsigned short pid, unsigned short slot) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->equipedItems[slot].refId.c_str();
}

int ItemFunctions::GetEquipmentItemCount(unsigned short pid, unsigned short slot) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->equipedItems[slot].count;
}

int ItemFunctions::GetEquipmentItemCharge(unsigned short pid, unsigned short slot) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->equipedItems[slot].charge;
}

const char *ItemFunctions::GetInventoryItemRefId(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, "");

    if (i >= player->inventoryChanges.count)
        return "invalid";

    return player->inventoryChanges.items.at(i).refId.c_str();
}

int ItemFunctions::GetInventoryItemCount(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->inventoryChanges.items.at(i).count;
}

int ItemFunctions::GetInventoryItemCharge(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->inventoryChanges.items.at(i).charge;
}

void ItemFunctions::SendEquipment(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_EQUIPMENT)->setPlayer(player);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_EQUIPMENT)->Send(false);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_EQUIPMENT)->Send(true);
}

void ItemFunctions::SendInventoryChanges(unsigned short pid, bool toOthers) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_INVENTORY)->setPlayer(player);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_INVENTORY)->Send(toOthers);
}
