#ifndef GAME_MWWORLD_INVENTORYSTORE_H
#define GAME_MWWORLD_INVENTORYSTORE_H

#include "containerstore.hpp"

namespace MWMechanics
{
    struct NpcStats;
}

namespace MWWorld
{
    ///< \brief Variant of the ContainerStore for NPCs
    class InventoryStore : public ContainerStore
    {
        public:

            static const int Slot_Helmet = 0;
            static const int Slot_Cuirass = 1;
            static const int Slot_Greaves = 2;
            static const int Slot_LeftPauldron = 3;
            static const int Slot_RightPauldron = 4;
            static const int Slot_LeftGauntlet = 5;
            static const int Slot_RightGauntlet = 6;
            static const int Slot_Boots = 7;
            static const int Slot_Shirt = 8;
            static const int Slot_Pants = 9;
            static const int Slot_Skirt = 10;
            static const int Slot_Robe = 11;
            static const int Slot_LeftRing = 12;
            static const int Slot_RightRing = 13;
            static const int Slot_Amulet = 14;
            static const int Slot_Belt = 15;
            static const int Slot_CarriedRight = 16;
            static const int Slot_CarriedLeft = 17;
            static const int Slot_Ammunition = 18;

            static const int Slots = 19;

            static const int Slot_NoSlot = -1;

        private:

            typedef std::vector<ContainerStoreIterator> TSlots;

            mutable TSlots mSlots;

            void copySlots (const InventoryStore& store);

            void initSlots (TSlots& slots);

        public:

            InventoryStore();

            InventoryStore (const InventoryStore& store);

            InventoryStore& operator= (const InventoryStore& store);

            void equip (int slot, const ContainerStoreIterator& iterator);
            ///< \note \a iteartor can be an end-iterator

            void _freeSlot(int slot);
            ///< this method is dangerous, as it doesn't do re-stacking items - you probably want to use equip()

            ContainerStoreIterator getSlot (int slot);

            void autoEquip (const MWMechanics::NpcStats& stats);
            ///< Auto equip items according to stats and item value.

        protected:

            virtual bool stacks (const Ptr& ptr1, const Ptr& ptr2);
            ///< @return true if the two specified objects can stack with each other
            /// @note ptr1 is the item that is already in this container

    };
}

#endif
