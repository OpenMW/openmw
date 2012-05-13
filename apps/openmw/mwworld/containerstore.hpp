#ifndef GAME_MWWORLD_CONTAINERSTORE_H
#define GAME_MWWORLD_CONTAINERSTORE_H

#include <iterator>

#include <components/esm_store/cell_store.hpp>

#include "refdata.hpp"
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

            ESMS::CellRefList<ESM::Potion, RefData>            potions;
            ESMS::CellRefList<ESM::Apparatus, RefData>         appas;
            ESMS::CellRefList<ESM::Armor, RefData>             armors;
            ESMS::CellRefList<ESM::Book, RefData>              books;
            ESMS::CellRefList<ESM::Clothing, RefData>          clothes;
            ESMS::CellRefList<ESM::Ingredient, RefData>        ingreds;
            ESMS::CellRefList<ESM::Light, RefData>             lights;
            ESMS::CellRefList<ESM::Tool, RefData>              lockpicks;
            ESMS::CellRefList<ESM::Miscellaneous, RefData>     miscItems;
            ESMS::CellRefList<ESM::Probe, RefData>             probes;
            ESMS::CellRefList<ESM::Repair, RefData>            repairs;
            ESMS::CellRefList<ESM::Weapon, RefData>            weapons;
            int mStateId;
            mutable float mCachedWeight;
            mutable bool mWeightUpToDate;

        public:

            ContainerStore();

            virtual ~ContainerStore();

            ContainerStoreIterator begin (int mask = Type_All);

            ContainerStoreIterator end();

            void add (const Ptr& ptr);
            ///< Add the item pointed to by \a ptr to this container. (Stacks automatically if needed)
            ///
            /// \note The item pointed to is not required to exist beyond this function call.
            ///
            /// \attention Do not add items to an existing stack by increasing the count instead of
            /// calling this function!

        protected:
            void addImpl (const Ptr& ptr);
            ///< Add the item to this container (no stacking)

            virtual bool stacks (const Ptr& ptr1, const Ptr& ptr2);
            ///< @return true if the two specified objects can stack with each other
            /// @note ptr1 is the item that is already in this container

        public:

            void fill (const ESM::InventoryList& items, const ESMS::ESMStore& store);
            ///< Insert items into *this.

            void clear();
            ///< Empty container.

            void flagAsModified();
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

            ESMS::CellRefList<ESM::Potion, RefData>::List::iterator mPotion;
            ESMS::CellRefList<ESM::Apparatus, RefData>::List::iterator mApparatus;
            ESMS::CellRefList<ESM::Armor, RefData>::List::iterator mArmor;
            ESMS::CellRefList<ESM::Book, RefData>::List::iterator mBook;
            ESMS::CellRefList<ESM::Clothing, RefData>::List::iterator mClothing;
            ESMS::CellRefList<ESM::Ingredient, RefData>::List::iterator mIngredient;
            ESMS::CellRefList<ESM::Light, RefData>::List::iterator mLight;
            ESMS::CellRefList<ESM::Tool, RefData>::List::iterator mLockpick;
            ESMS::CellRefList<ESM::Miscellaneous, RefData>::List::iterator mMiscellaneous;
            ESMS::CellRefList<ESM::Probe, RefData>::List::iterator mProbe;
            ESMS::CellRefList<ESM::Repair, RefData>::List::iterator mRepair;
            ESMS::CellRefList<ESM::Weapon, RefData>::List::iterator mWeapon;

        private:

            ContainerStoreIterator (ContainerStore *container);
            ///< End-iterator

            ContainerStoreIterator (int mask, ContainerStore *container);
            ///< Begin-iterator

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
