//
// Created by koncord on 30.08.16.
//

#ifndef OPENMW_ITEMS_HPP
#define OPENMW_ITEMS_HPP

#define ITEMAPI \
    {"addItem",           ItemFunctions::addItem},\
    {"removeItem",        ItemFunctions::removeItem},\
    {"getItemCount",      ItemFunctions::getItemCount2},\
    {"equipItem",         ItemFunctions::equipItem},\
    {"unequipItem",       ItemFunctions::unequipItem},\
    {"getItemSlot",       ItemFunctions::getItemSlot},\
    {"hasItemEquipped",   ItemFunctions::hasItemEquipped},\
    {"getItemName",       ItemFunctions::getItemName},\
    {"getItemHealth",     ItemFunctions::getItemHealth},\
    {"getInventorySize",  ItemFunctions::getInventorySize},\
    \
    {"sendEquipment",     ItemFunctions::sendEquipment},\
    {"sendInventory",     ItemFunctions::sendInventory}\

class ItemFunctions
{
public:
    static void addItem(unsigned short pid, const char* itemName, unsigned int count, int health) noexcept;
    static void removeItem(unsigned short pid, const char* itemName, unsigned short count) noexcept;
    static void getItemCount(unsigned short pid, const char* itemName) noexcept;
    static void equipItem(unsigned short pid, unsigned short slot, const char* itemName, unsigned int count, int health) noexcept;
    static void unequipItem(unsigned short pid, unsigned short slot) noexcept;
    static bool hasItemEquipped(unsigned short pid, const char* itemName);
    static const char *getItemSlot(unsigned short pid, unsigned short slot) noexcept;

    static const char *getItemName(unsigned short pid, unsigned int i) noexcept;
    static int getItemCount2(unsigned short pid, unsigned int i) noexcept;
    static int getItemHealth(unsigned short pid, unsigned int i) noexcept;
    static unsigned int getInventorySize(unsigned short pid) noexcept;

    static void sendEquipment(unsigned short pid) noexcept;
    static void sendInventory(unsigned short pid) noexcept;
private:

};

#endif //OPENMW_ITEMS_HPP
