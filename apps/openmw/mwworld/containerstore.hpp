#ifndef GAME_MWWORLD_CONTAINERSTORE_H
#define GAME_MWWORLD_CONTAINERSTORE_H

#include <iterator>

#include "ptr.hpp"

namespace ESM
{
    struct InventoryList;
}

namespace MWWorld
{
    class ContainerStoreIterator;

    class ContainerStore
    {
        public:

            static const int Type_Potion = 0x0001;
            static const int Type_Apparatus = 0x0002;
            static const int Type_Armor = 0x0004;
            static const int Type_Book = 0x0008;
            static const int Type_Clothing = 0x0010;
            static const int Type_Ingredient = 0x0020;
            static const int Type_Light = 0x0040;
            static const int Type_Lockpick = 0x0080;
            static const int Type_Miscellaneous = 0x0100;
            static const int Type_Probe = 0x0200;
            static const int Type_Repair = 0x0400;
            static const int Type_Weapon = 0x0800;

            static const int Type_Last = Type_Weapon;

            static const int Type_All = 0xffff;

        private:

            MWWorld::ContainerRefList<ESM::Potion>            potions;
            MWWorld::ContainerRefList<ESM::Apparatus>         appas;
            MWWorld::ContainerRefList<ESM::Armor>             armors;
            MWWorld::ContainerRefList<ESM::Book>              books;
            MWWorld::ContainerRefList<ESM::Clothing>          clothes;
            MWWorld::ContainerRefList<ESM::Ingredient>        ingreds;
            MWWorld::ContainerRefList<ESM::Light>             lights;
            MWWorld::ContainerRefList<ESM::Tool>              lockpicks;
            MWWorld::ContainerRefList<ESM::Miscellaneous>     miscItems;
            MWWorld::ContainerRefList<ESM::Probe>             probes;
            MWWorld::ContainerRefList<ESM::Repair>            repairs;
            MWWorld::ContainerRefList<ESM::Weapon>            weapons;
            int mStateId;
            mutable float mCachedWeight;
            mutable bool mWeightUpToDate;

        public:

            ContainerStore();

            virtual ~ContainerStore();

            ContainerStoreIterator begin (int mask = Type_All);

            ContainerStoreIterator end();

            ContainerStoreIterator add (const Ptr& ptr);
            ///< Add the item pointed to by \a ptr to this container. (Stacks automatically if needed)
            ///
            /// \note The item pointed to is not required to exist beyond this function call.
            ///
            /// \attention Do not add items to an existing stack by increasing the count instead of
            /// calling this function!
            ///
            /// @return if stacking happened, return iterator to the item that was stacked against, otherwise iterator to the newly inserted item.

        protected:
            ContainerStoreIterator addImpl (const Ptr& ptr);
            ///< Add the item to this container (no stacking)

            virtual bool stacks (const Ptr& ptr1, const Ptr& ptr2);
            ///< @return true if the two specified objects can stack with each other
            /// @note ptr1 is the item that is already in this container

        public:

            void fill (const ESM::InventoryList& items, const ESMS::ESMStore& store);
            ///< Insert items into *this.

            void clear();
            ///< Empty container.

            virtual void flagAsModified();
            ///< \attention This function is internal to the world model and should not be called from
            /// outside.

            int getStateId() const;
            ///< This ID is changed every time the container is modified or items in the container
            /// are accessed in a way that may be used to modify the item.
            /// \note This method of change-tracking will ocasionally yield false positives.

            float getWeight() const;
            ///< Return total weight of the items contained in *this.

            static int getType (const Ptr& ptr);
            ///< This function throws an exception, if ptr does not point to an object, that can be
            /// put into a container.

        friend class ContainerStoreIterator;
    };

    /// \brief Iteration over a subset of objects in a ContainerStore
    ///
    /// \note The iterator will automatically skip over deleted objects.
    class ContainerStoreIterator
        : public std::iterator<std::forward_iterator_tag, Ptr, std::ptrdiff_t, Ptr *, Ptr&>
    {
            int mType;
            int mMask;
            ContainerStore *mContainer;
            mutable Ptr mPtr;

            MWWorld::ContainerRefList<ESM::Potion>::List::iterator mPotion;
            MWWorld::ContainerRefList<ESM::Apparatus>::List::iterator mApparatus;
            MWWorld::ContainerRefList<ESM::Armor>::List::iterator mArmor;
            MWWorld::ContainerRefList<ESM::Book>::List::iterator mBook;
            MWWorld::ContainerRefList<ESM::Clothing>::List::iterator mClothing;
            MWWorld::ContainerRefList<ESM::Ingredient>::List::iterator mIngredient;
            MWWorld::ContainerRefList<ESM::Light>::List::iterator mLight;
            MWWorld::ContainerRefList<ESM::Tool>::List::iterator mLockpick;
            MWWorld::ContainerRefList<ESM::Miscellaneous>::List::iterator mMiscellaneous;
            MWWorld::ContainerRefList<ESM::Probe>::List::iterator mProbe;
            MWWorld::ContainerRefList<ESM::Repair>::List::iterator mRepair;
            MWWorld::ContainerRefList<ESM::Weapon>::List::iterator mWeapon;

        private:

            ContainerStoreIterator (ContainerStore *container);
            ///< End-iterator

            ContainerStoreIterator (int mask, ContainerStore *container);
            ///< Begin-iterator

            // construct iterator using a CellRefList iterator
            ContainerStoreIterator (ContainerStore *container, MWWorld::ContainerRefList<ESM::Potion>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::ContainerRefList<ESM::Apparatus>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::ContainerRefList<ESM::Armor>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::ContainerRefList<ESM::Book>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::ContainerRefList<ESM::Clothing>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::ContainerRefList<ESM::Ingredient>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::ContainerRefList<ESM::Light>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::ContainerRefList<ESM::Tool>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::ContainerRefList<ESM::Miscellaneous>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::ContainerRefList<ESM::Probe>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::ContainerRefList<ESM::Repair>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::ContainerRefList<ESM::Weapon>::List::iterator);

            void incType();

            void nextType();

            bool resetIterator();
            ///< Reset iterator for selected type.
            ///
            /// \return Type not empty?

            bool incIterator();
            ///< Increment iterator for selected type.
            ///
            /// \return reached the end?

        public:

            Ptr *operator->() const;

            Ptr operator*() const;

            ContainerStoreIterator& operator++();

            ContainerStoreIterator operator++ (int);

            bool isEqual (const ContainerStoreIterator& iter) const;

            int getType() const;

            const ContainerStore *getContainerStore() const;

        friend class ContainerStore;
    };

    bool operator== (const ContainerStoreIterator& left, const ContainerStoreIterator& right);
    bool operator!= (const ContainerStoreIterator& left, const ContainerStoreIterator& right);
}

#endif
