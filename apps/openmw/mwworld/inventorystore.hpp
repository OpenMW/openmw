#ifndef GAME_MWWORLD_INVENTORYSTORE_H
#define GAME_MWWORLD_INVENTORYSTORE_H

#include "containerstore.hpp"

#include "../mwmechanics/magiceffects.hpp"

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
        virtual void equipmentChanged () {}

        /**
         * @param effect
         * @param isNew Is this effect new (e.g. the item for it was just now manually equipped)
         *              or was it loaded from a savegame / initial game state? \n
         *              If it isn't new, non-looping VFX should not be played.
         * @param playSound Play effect sound?
         */
        virtual void permanentEffectAdded (const ESM::MagicEffect *magicEffect, bool isNew) {}

        virtual ~InventoryStoreListener() = default;
    };

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

            MWMechanics::MagicEffects mMagicEffects;

            InventoryStoreListener* mInventoryListener;

            // Enables updates of magic effects and actor model whenever items are equipped or unequipped.
            // This is disabled during autoequip to avoid excessive updates
            bool mUpdatesEnabled;

            bool mFirstAutoEquip;

            // Vanilla allows permanent effects with a random magnitude, so it needs to be stored here.
            // We also need this to only play sounds and particle effects when the item is equipped, rather than on every update.
            struct EffectParams
            {
                // Modifier to scale between min and max magnitude
                float mRandom;
                // Multiplier for when an effect was fully or partially resisted
                float mMultiplier;
            };

            typedef std::map<std::string, std::vector<EffectParams> > TEffectMagnitudes;
            TEffectMagnitudes mPermanentMagicEffectMagnitudes;

            typedef std::vector<ContainerStoreIterator> TSlots;

            TSlots mSlots;

            void autoEquipWeapon(const MWWorld::Ptr& actor, TSlots& slots_);
            void autoEquipArmor(const MWWorld::Ptr& actor, TSlots& slots_);
            void autoEquipShield(const MWWorld::Ptr& actor, TSlots& slots_);

            // selected magic item (for using enchantments of type "Cast once" or "Cast when used")
            ContainerStoreIterator mSelectedEnchantItem;

            void copySlots (const InventoryStore& store);

            void initSlots (TSlots& slots_);

            void updateMagicEffects(const Ptr& actor);

            void fireEquipmentChangedEvent(const Ptr& actor);

            virtual void storeEquipmentState (const MWWorld::LiveCellRefBase& ref, int index, ESM::InventoryState& inventory) const;
            virtual void readEquipmentState (const MWWorld::ContainerStoreIterator& iter, int index, const ESM::InventoryState& inventory);

            ContainerStoreIterator findSlot (int slot) const;

        public:

            InventoryStore();

            InventoryStore (const InventoryStore& store);

            InventoryStore& operator= (const InventoryStore& store);

            virtual InventoryStore* clone() { return new InventoryStore(*this); }

            virtual ContainerStoreIterator add (const Ptr& itemPtr, int count, const Ptr& actorPtr, bool allowAutoEquip = true);
            ///< Add the item pointed to by \a ptr to this container. (Stacks automatically if needed)
            /// Auto-equip items if specific conditions are fulfilled and allowAutoEquip is true (see the implementation).
            ///
            /// \note The item pointed to is not required to exist beyond this function call.
            ///
            /// \attention Do not add items to an existing stack by increasing the count instead of
            /// calling this function!
            ///
            /// @return if stacking happened, return iterator to the item that was stacked against, otherwise iterator to the newly inserted item.

            void equip (int slot, const ContainerStoreIterator& iterator, const Ptr& actor);
            ///< \warning \a iterator can not be an end()-iterator, use unequip function instead

            bool isEquipped(const MWWorld::ConstPtr& item);
            ///< Utility function, returns true if the given item is equipped in any slot

            void setSelectedEnchantItem(const ContainerStoreIterator& iterator);
            ///< set the selected magic item (for using enchantments of type "Cast once" or "Cast when used")
            /// \note to unset the selected item, call this method with end() iterator

            ContainerStoreIterator getSelectedEnchantItem();
            ///< @return selected magic item (for using enchantments of type "Cast once" or "Cast when used")
            /// \note if no item selected, return end() iterator

            ContainerStoreIterator getSlot (int slot);
            ConstContainerStoreIterator getSlot(int slot) const;

            void unequipAll(const MWWorld::Ptr& actor);
            ///< Unequip all currently equipped items.

            void autoEquip (const MWWorld::Ptr& actor);
            ///< Auto equip items according to stats and item value.

            const MWMechanics::MagicEffects& getMagicEffects() const;
            ///< Return magic effects from worn items.

            virtual bool stacks (const ConstPtr& ptr1, const ConstPtr& ptr2) const;
            ///< @return true if the two specified objects can stack with each other

            virtual int remove(const std::string& itemId, int count, const Ptr& actor);
            virtual int remove(const std::string& itemId, int count, const Ptr& actor, bool equipReplacement);

            virtual int remove(const Ptr& item, int count, const Ptr& actor);
            virtual int remove(const Ptr& item, int count, const Ptr& actor, bool equipReplacement);
            ///< Remove \a count item(s) designated by \a item from this inventory.
            ///
            /// @return the number of items actually removed

            ContainerStoreIterator unequipSlot(int slot, const Ptr& actor, bool fireEvent=true);
            ///< Unequip \a slot.
            ///
            /// @return an iterator to the item that was previously in the slot

            ContainerStoreIterator unequipItem(const Ptr& item, const Ptr& actor);
            ///< Unequip an item identified by its Ptr. An exception is thrown
            /// if the item is not currently equipped.
            ///
            /// @return an iterator to the item that was previously in the slot
            /// (it can be re-stacked so its count may be different than when it
            /// was equipped).

            ContainerStoreIterator unequipItemQuantity(const Ptr& item, const Ptr& actor, int count);
            ///< Unequip a specific quantity of an item identified by its Ptr.
            /// An exception is thrown if the item is not currently equipped,
            /// if count <= 0, or if count > the item stack size.
            ///
            /// @return an iterator to the unequipped items that were previously
            /// in the slot (they can be re-stacked so its count may be different
            /// than the requested count).

            void setInvListener (InventoryStoreListener* listener, const Ptr& actor);
            ///< Set a listener for various events, see \a InventoryStoreListener

            InventoryStoreListener* getInvListener();

            void visitEffectSources (MWMechanics::EffectSourceVisitor& visitor);

            void purgeEffect (short effectId);
            ///< Remove a magic effect

            void purgeEffect (short effectId, const std::string& sourceId);
            ///< Remove a magic effect

            virtual void clear();
            ///< Empty container.

            virtual void writeState (ESM::InventoryState& state) const;

            virtual void readState (const ESM::InventoryState& state);
    };
}

#endif
