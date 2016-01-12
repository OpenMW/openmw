//
// Created by koncord on 02.03.16.
//

#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <apps/openmw-mp/Networking.hpp>
#include <components/misc/stringops.hpp>


/*void ScriptFunctions::AddItem(unsigned short pid, const char* itemName, unsigned short count) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);



    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_UPDATE_BASESTATS)->Send(player, false);
    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_UPDATE_BASESTATS)->Send(player, true);
}*/

void ScriptFunctions::EquipItem(unsigned short pid, unsigned short slot, const char *itemName, unsigned short count) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->EquipedItem(slot)->refid = itemName;
    player->EquipedItem(slot)->count = count;

    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_UPDATE_EQUIPED)->Send(player, false);
    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_UPDATE_EQUIPED)->Send(player, true);
}

void ScriptFunctions::UnequipItem(unsigned short pid, unsigned short slot) noexcept
{
    ScriptFunctions::EquipItem(pid, slot, "", 0);
}

const char *ScriptFunctions::GetItemSlot(unsigned short pid, unsigned short slot) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->EquipedItem(slot)->refid.c_str();
}

bool ScriptFunctions::HasItemEquipped(unsigned short pid, const char* itemName)
{
    Player *player;
    GET_PLAYER(pid, player, false);

    for(int slot = 0; slot < 27; slot ++)
        if(Misc::StringUtils::ciEqual(player->EquipedItem(slot)->refid, itemName))
            return true;
    return false;
}