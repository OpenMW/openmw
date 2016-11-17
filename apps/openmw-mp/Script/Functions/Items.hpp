//
// Created by koncord on 30.08.16.
//

#ifndef OPENMW_ITEMS_HPP
#define OPENMW_ITEMS_HPP

#define ITEMAPI \
    {"GetEquipmentSize",       ItemFunctions::GetEquipmentSize},\
    {"GetInventorySize",       ItemFunctions::GetInventorySize},\
    \
    {"EquipItem",              ItemFunctions::EquipItem},\
    {"UnequipItem",            ItemFunctions::UnequipItem},\
    \
    {"AddItem",                ItemFunctions::AddItem}, \
    {"RemoveItem",             ItemFunctions::RemoveItem}, \
    \
    {"HasItemEquipped",        ItemFunctions::HasItemEquipped},\
    \
    {"GetEquipmentItemId",     ItemFunctions::GetEquipmentItemId},\
    {"GetEquipmentItemCount",  ItemFunctions::GetEquipmentItemCount},\
    {"GetEquipmentItemHealth", ItemFunctions::GetEquipmentItemHealth},\
    \
    {"GetInventoryItemId",     ItemFunctions::GetInventoryItemId},\
    {"GetInventoryItemCount",  ItemFunctions::GetInventoryItemCount},\
    {"GetInventoryItemHealth", ItemFunctions::GetInventoryItemHealth},\
    \
    {"SendEquipment",          ItemFunctions::SendEquipment},\
    {"SendInventory",          ItemFunctions::SendInventory}

class ItemFunctions
{
public:

    static int GetEquipmentSize() noexcept;
    static unsigned int GetInventorySize(unsigned short pid) noexcept;

    static void EquipItem(unsigned short pid, unsigned short slot, const char* itemName, unsigned int count, int health) noexcept;
    static void UnequipItem(unsigned short pid, unsigned short slot) noexcept;

    static void AddItem(unsigned short pid, const char* itemName, unsigned int count, int health) noexcept;
    static void RemoveItem(unsigned short pid, const char* itemName, unsigned short count) noexcept;

    static bool HasItemEquipped(unsigned short pid, const char* itemName);

    static const char *GetEquipmentItemId(unsigned short pid, unsigned short slot) noexcept;
    static int GetEquipmentItemCount(unsigned short pid, unsigned short slot) noexcept;
    static int GetEquipmentItemHealth(unsigned short pid, unsigned short slot) noexcept;

    static const char *GetInventoryItemId(unsigned short pid, unsigned int i) noexcept;
    static int GetInventoryItemCount(unsigned short pid, unsigned int i) noexcept;
    static int GetInventoryItemHealth(unsigned short pid, unsigned int i) noexcept;

    static void SendEquipment(unsigned short pid) noexcept;
    static void SendInventory(unsigned short pid) noexcept;
private:

};

#endif //OPENMW_ITEMS_HPP
