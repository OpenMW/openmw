//
// Created by koncord on 02.03.16.
//

#include "Items.hpp"
#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <apps/openmw-mp/Networking.hpp>
#include <components/misc/stringops.hpp>


void ItemFunctions::AddItem(unsigned short pid, const char* itemName, unsigned short count) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    LOG_MESSAGE(Log::LOG_WARN, "%s", "stub");
}

void ItemFunctions::EquipItem(unsigned short pid, unsigned short slot, const char *itemName, unsigned short count) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->EquipedItem(slot)->refid = itemName;
    player->EquipedItem(slot)->count = count;
}

void ItemFunctions::UnequipItem(unsigned short pid, unsigned short slot) noexcept
{
    LOG_MESSAGE(Log::LOG_WARN, "%s", "stub");
    //ItemFunctions::EquipItem(pid, slot, "", 0);
}

const char *ItemFunctions::GetItemSlot(unsigned short pid, unsigned short slot) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->EquipedItem(slot)->refid.c_str();
}

bool ItemFunctions::HasItemEquipped(unsigned short pid, const char* itemName)
{
    Player *player;
    GET_PLAYER(pid, player, false);

    for (int slot = 0; slot < 27; slot ++)
        if (Misc::StringUtils::ciEqual(player->EquipedItem(slot)->refid, itemName))
            return true;
    return false;
}

void ItemFunctions::RemoveItem(unsigned short pid, const char* itemName, unsigned short count) noexcept
{
    LOG_MESSAGE(Log::LOG_WARN, "%s", "stub");
}
void ItemFunctions::GetItemCount(unsigned short pid, const char* itemName) noexcept
{
    LOG_MESSAGE(Log::LOG_WARN, "%s", "stub");
}

void ItemFunctions::SendEquipment(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_EQUIPMENT)->Send(player, false);
    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_EQUIPMENT)->Send(player, true);
}
