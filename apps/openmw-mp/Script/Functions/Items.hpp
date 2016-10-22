//
// Created by koncord on 30.08.16.
//

#ifndef OPENMW_ITEMS_HPP
#define OPENMW_ITEMS_HPP

#define ITEMAPI \
    {"AddItem",           ItemFunctions::AddItem},\
    {"RemoveItem",        ItemFunctions::RemoveItem},\
    {"GetItemCount",      ItemFunctions::GetItemCount2},\
    {"EquipItem",         ItemFunctions::EquipItem},\
    {"UnequipItem",       ItemFunctions::UnequipItem},\
    {"GetItemSlot",       ItemFunctions::GetItemSlot},\
    {"HasItemEquipped",   ItemFunctions::HasItemEquipped},\
    {"GetItemName",       ItemFunctions::GetItemName},\
    {"GetItemHealth",     ItemFunctions::GetItemHealth},\
    {"GetInventorySize",  ItemFunctions::GetInventorySize},\
    \
    {"SendEquipment",     ItemFunctions::SendEquipment},\
    {"SendInventory",     ItemFunctions::SendInventory}\

class ItemFunctions
{
public:
    static void AddItem(unsigned short pid, const char* itemName, unsigned int count, int health) noexcept;
    static void RemoveItem(unsigned short pid, const char* itemName, unsigned short count) noexcept;
    static void GetItemCount(unsigned short pid, const char* itemName) noexcept;
    static void EquipItem(unsigned short pid, unsigned short slot, const char* itemName, unsigned int count, int health) noexcept;
    static void UnequipItem(unsigned short pid, unsigned short slot) noexcept;
    static bool HasItemEquipped(unsigned short pid, const char* itemName);
    static const char *GetItemSlot(unsigned short pid, unsigned short slot) noexcept;

    static const char *GetItemName(unsigned short pid, unsigned int i) noexcept;
    static int GetItemCount2(unsigned short pid, unsigned int i) noexcept;
    static int GetItemHealth(unsigned short pid, unsigned int i) noexcept;
    static unsigned int GetInventorySize(unsigned short pid) noexcept;

    static void SendEquipment(unsigned short pid) noexcept;
    static void SendInventory(unsigned short pid) noexcept;
private:

};

#endif //OPENMW_ITEMS_HPP
