#ifndef GAME_MWWORLD_INVENTORYSTORE_H
#define GAME_MWWORLD_INVENTORYSTORE_H

#include "containerstore.hpp"

namespace ESM
{
    struct MagicEffect;
}

namespace MWMechanics
{
    class NpcStats;
}

namespace MWWorld
{
    class InventoryStoreListener
    {
    public:
        /**
         * Fired when items are equipped or unequipped
         */
        virtual void equipmentChanged() {}

        virtual ~InventoryStoreListener() = default;
    };

    ///< \brief Variant of the ContainerStore for NPCs
    class InventoryStore : public ContainerStore
    {
    public:
        static constexpr int Slot_Helmet = 0;
        static constexpr int Slot_Cuirass = 1;
        static constexpr int Slot_Greaves = 2;
        static constexpr int Slot_LeftPauldron = 3;
        static constexpr int Slot_RightPauldron = 4;
        static constexpr int Slot_LeftGauntlet = 5;
        static constexpr int Slot_RightGauntlet = 6;
        static constexpr int Slot_Boots = 7;
        static constexpr int Slot_Shirt = 8;
        static constexpr int Slot_Pants = 9;
        static constexpr int Slot_Skirt = 10;
        static constexpr int Slot_Robe = 11;
        static constexpr int Slot_LeftRing = 12;
        static constexpr int Slot_RightRing = 13;
        static constexpr int Slot_Amulet = 14;
        static constexpr int Slot_Belt = 15;
        static constexpr int Slot_CarriedRight = 16;
        static constexpr int Slot_CarriedLeft = 17;
        static constexpr int Slot_Ammunition = 18;

        static constexpr int Slots = 19;

        static constexpr int Slot_NoSlot = -1;

    private:
        InventoryStoreListener* mInventoryListener;

        // Enables updates of magic effects and actor model whenever items are equipped or unequipped.
        // This is disabled during autoequip to avoid excessive updates
        bool mUpdatesEnabled;

        bool mFirstAutoEquip;

        typedef std::vector<ContainerStoreIterator> TSlots;

        TSlots mSlots;

        void autoEquipWeapon(TSlots& slots_);
        void autoEquipArmor(TSlots& slots_);

        // selected magic item (for using enchantments of type "Cast once" or "Cast when used")
        ContainerStoreIterator mSelectedEnchantItem;

        void copySlots(const InventoryStore& store);

        void initSlots(TSlots& slots_);

        void fireEquipmentChangedEvent();

        void storeEquipmentState(
            const MWWorld::LiveCellRefBase& ref, size_t index, ESM::InventoryState& inventory) const override;
        void readEquipmentState(
            const MWWorld::ContainerStoreIterator& iter, size_t index, const ESM::InventoryState& inventory) override;

        ContainerStoreIterator findSlot(int slot) const;

    public:
        InventoryStore();

        InventoryStore(const InventoryStore& store);

        InventoryStore& operator=(const InventoryStore& store);

        std::unique_ptr<ContainerStore> clone() override
        {
            auto res = std::make_unique<InventoryStore>(*this);
            res->updateRefNums();
            return res;
        }

        ContainerStoreIterator add(
            const Ptr& itemPtr, int count, bool allowAutoEquip = true, bool resolve = true) override;
        ///< Add the item pointed to by \a ptr to this container. (Stacks automatically if needed)
        /// Auto-equip items if specific conditions are fulfilled and allowAutoEquip is true (see the implementation).
        ///
        /// \note The item pointed to is not required to exist beyond this function call.
        ///
        /// \attention Do not add items to an existing stack by increasing the count instead of
        /// calling this function!
        ///
        /// @return if stacking happened, return iterator to the item that was stacked against, otherwise iterator to
        /// the newly inserted item.

        void equip(int slot, const ContainerStoreIterator& iterator);
        ///< \warning \a iterator can not be an end()-iterator, use unequip function instead

        bool isEquipped(const MWWorld::ConstPtr& item);
        ///< Utility function, returns true if the given item is equipped in any slot

        void setSelectedEnchantItem(const ContainerStoreIterator& iterator);
        ///< set the selected magic item (for using enchantments of type "Cast once" or "Cast when used")
        /// \note to unset the selected item, call this method with end() iterator

        ContainerStoreIterator getSelectedEnchantItem();
        ///< @return selected magic item (for using enchantments of type "Cast once" or "Cast when used")
        /// \note if no item selected, return end() iterator

        ContainerStoreIterator getSlot(int slot);
        ConstContainerStoreIterator getSlot(int slot) const;

        ContainerStoreIterator getPreferredShield();

        void unequipAll();
        ///< Unequip all currently equipped items.

        void autoEquip();
        ///< Auto equip items according to stats and item value.

        bool stacks(const ConstPtr& ptr1, const ConstPtr& ptr2) const override;
        ///< @return true if the two specified objects can stack with each other

        using ContainerStore::remove;
        int remove(const Ptr& item, int count, bool equipReplacement = 0, bool resolve = true) override;
        ///< Remove \a count item(s) designated by \a item from this inventory.
        ///
        /// @return the number of items actually removed

        ContainerStoreIterator unequipSlot(int slot, bool applyUpdates = true);
        ///< Unequip \a slot.
        ///
        /// @return an iterator to the item that was previously in the slot

        ContainerStoreIterator unequipItem(const Ptr& item);
        ///< Unequip an item identified by its Ptr. An exception is thrown
        /// if the item is not currently equipped.
        ///
        /// @return an iterator to the item that was previously in the slot
        /// (it can be re-stacked so its count may be different than when it
        /// was equipped).

        ContainerStoreIterator unequipItemQuantity(const Ptr& item, int count);
        ///< Unequip a specific quantity of an item identified by its Ptr.
        /// An exception is thrown if the item is not currently equipped,
        /// if count <= 0, or if count > the item stack size.
        ///
        /// @return an iterator to the unequipped items that were previously
        /// in the slot (they can be re-stacked so its count may be different
        /// than the requested count).

        void setInvListener(InventoryStoreListener* listener);
        ///< Set a listener for various events, see \a InventoryStoreListener

        InventoryStoreListener* getInvListener() const;

        void clear() override;
        ///< Empty container.

        bool isFirstEquip();
    };
}

#endif
