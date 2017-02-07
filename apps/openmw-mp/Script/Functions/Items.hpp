//
// Created by koncord on 30.08.16.
//

#ifndef OPENMW_ITEMS_HPP
#define OPENMW_ITEMS_HPP

#define ITEMAPI \
    {"GetEquipmentSize",        ItemFunctions::GetEquipmentSize},\
    {"GetInventoryChangesSize", ItemFunctions::GetInventoryChangesSize},\
    \
    {"EquipItem",               ItemFunctions::EquipItem},\
    {"UnequipItem",             ItemFunctions::UnequipItem},\
    \
    {"AddItem",                 ItemFunctions::AddItem},\
    {"RemoveItem",              ItemFunctions::RemoveItem},\
    {"ClearInventory",          ItemFunctions::ClearInventory},\
    \
    {"HasItemEquipped",         ItemFunctions::HasItemEquipped},\
    \
    {"GetEquipmentItemRefId",   ItemFunctions::GetEquipmentItemRefId},\
    {"GetEquipmentItemCount",   ItemFunctions::GetEquipmentItemCount},\
    {"GetEquipmentItemCharge",  ItemFunctions::GetEquipmentItemCharge},\
    \
    {"GetInventoryItemRefId",   ItemFunctions::GetInventoryItemRefId},\
    {"GetInventoryItemCount",   ItemFunctions::GetInventoryItemCount},\
    {"GetInventoryItemCharge",  ItemFunctions::GetInventoryItemCharge},\
    \
    {"SendEquipment",           ItemFunctions::SendEquipment},\
    {"SendInventoryChanges",    ItemFunctions::SendInventoryChanges}

class ItemFunctions
{
public:

    static int GetEquipmentSize() noexcept;
    static unsigned int GetInventoryChangesSize(unsigned short pid) noexcept;

    static void EquipItem(unsigned short pid, unsigned short slot, const char* refId, unsigned int count, int charge) noexcept;
    static void UnequipItem(unsigned short pid, unsigned short slot) noexcept;

    static void AddItem(unsigned short pid, const char* refId, unsigned int count, int charge) noexcept;
    static void RemoveItem(unsigned short pid, const char* refId, unsigned short count) noexcept;
    static void ClearInventory(unsigned short pid) noexcept;

    static bool HasItemEquipped(unsigned short pid, const char* refId);

    static const char *GetEquipmentItemRefId(unsigned short pid, unsigned short slot) noexcept;
    static int GetEquipmentItemCount(unsigned short pid, unsigned short slot) noexcept;
    static int GetEquipmentItemCharge(unsigned short pid, unsigned short slot) noexcept;

    static const char *GetInventoryItemRefId(unsigned short pid, unsigned int i) noexcept;
    static int GetInventoryItemCount(unsigned short pid, unsigned int i) noexcept;
    static int GetInventoryItemCharge(unsigned short pid, unsigned int i) noexcept;

    static void SendEquipment(unsigned short pid) noexcept;
    static void SendInventoryChanges(unsigned short pid) noexcept;
private:

};

#endif //OPENMW_ITEMS_HPP
