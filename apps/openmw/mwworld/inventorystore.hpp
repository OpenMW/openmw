#ifndef GAME_MWWORLD_INVENTORYSTORE_H
#define GAME_MWWORLD_INVENTORYSTORE_H

#include "containerstore.hpp"

#include "../mwmechanics/magiceffects.hpp"

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

            mutable MWMechanics::MagicEffects mMagicEffects;
            mutable bool mMagicEffectsUpToDate;

            typedef std::vector<ContainerStoreIterator> TSlots;

            mutable TSlots mSlots;

            // selected magic item (for using enchantments of type "Cast once" or "Cast when used")
            ContainerStoreIterator mSelectedEnchantItem;

            void copySlots (const InventoryStore& store);

            void initSlots (TSlots& slots);

        public:

            InventoryStore();

            InventoryStore (const InventoryStore& store);

            InventoryStore& operator= (const InventoryStore& store);

            void equip (int slot, const ContainerStoreIterator& iterator);
            ///< \note \a iterator can be an end-iterator

            void setSelectedEnchantItem(const ContainerStoreIterator& iterator);
            ///< set the selected magic item (for using enchantments of type "Cast once" or "Cast when used")
            /// \note to unset the selected item, call this method with end() iterator

            ContainerStoreIterator getSelectedEnchantItem();
            ///< @return selected magic item (for using enchantments of type "Cast once" or "Cast when used")
            /// \note if no item selected, return end() iterator

            ContainerStoreIterator getSlot (int slot);

            void autoEquip (const MWMechanics::NpcStats& stats);
            ///< Auto equip items according to stats and item value.

            const MWMechanics::MagicEffects& getMagicEffects();
            ///< Return magic effects from worn items.
            ///
            /// \todo make this const again, after the constness of Ptrs and iterators has been addressed.

            virtual void flagAsModified();
            ///< \attention This function is internal to the world model and should not be called from
            /// outside.

        protected:

            virtual bool stacks (const Ptr& ptr1, const Ptr& ptr2);
            ///< @return true if the two specified objects can stack with each other
            /// @note ptr1 is the item that is already in this container

    };
}

#endif
