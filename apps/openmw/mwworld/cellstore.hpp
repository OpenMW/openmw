#ifndef GAME_MWWORLD_CELLSTORE_H
#define GAME_MWWORLD_CELLSTORE_H

#include <algorithm>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <map>
#include <memory>

#include "livecellref.hpp"
#include "cellreflist.hpp"

#include <components/esm/loadacti.hpp>
#include <components/esm/loadalch.hpp>
#include <components/esm/loadappa.hpp>
#include <components/esm/loadarmo.hpp>
#include <components/esm/loadbook.hpp>
#include <components/esm/loadclot.hpp>
#include <components/esm/loadcont.hpp>
#include <components/esm/loadcrea.hpp>
#include <components/esm/loaddoor.hpp>
#include <components/esm/loadingr.hpp>
#include <components/esm/loadlevlist.hpp>
#include <components/esm/loadligh.hpp>
#include <components/esm/loadlock.hpp>
#include <components/esm/loadprob.hpp>
#include <components/esm/loadrepa.hpp>
#include <components/esm/loadstat.hpp>
#include <components/esm/loadweap.hpp>
#include <components/esm/loadnpc.hpp>
#include <components/esm/loadmisc.hpp>
#include <components/esm/loadbody.hpp>

#include "timestamp.hpp"
#include "ptr.hpp"

namespace ESM
{
    struct Cell;
    struct CellState;
    struct FogState;
    struct CellId;
}

namespace MWWorld
{
    class ESMStore;

    /// \brief Mutable state of a cell
    class CellStore
    {
        public:

            enum State
            {
                State_Unloaded, State_Preloaded, State_Loaded
            };

        private:

            const MWWorld::ESMStore& mStore;
            std::vector<ESM::ESMReader>& mReader;

            // Even though fog actually belongs to the player and not cells,
            // it makes sense to store it here since we need it once for each cell.
            // Note this is nullptr until the cell is explored to save some memory
            std::shared_ptr<ESM::FogState> mFogState;

            const ESM::Cell *mCell;
            State mState;
            bool mHasState;
            std::vector<std::string> mIds;
            float mWaterLevel;

            MWWorld::TimeStamp mLastRespawn;

            // List of refs owned by this cell
            CellRefList<ESM::Activator>         mActivators;
            CellRefList<ESM::Potion>            mPotions;
            CellRefList<ESM::Apparatus>         mAppas;
            CellRefList<ESM::Armor>             mArmors;
            CellRefList<ESM::Book>              mBooks;
            CellRefList<ESM::Clothing>          mClothes;
            CellRefList<ESM::Container>         mContainers;
            CellRefList<ESM::Creature>          mCreatures;
            CellRefList<ESM::Door>              mDoors;
            CellRefList<ESM::Ingredient>        mIngreds;
            CellRefList<ESM::CreatureLevList>   mCreatureLists;
            CellRefList<ESM::ItemLevList>       mItemLists;
            CellRefList<ESM::Light>             mLights;
            CellRefList<ESM::Lockpick>          mLockpicks;
            CellRefList<ESM::Miscellaneous>     mMiscItems;
            CellRefList<ESM::NPC>               mNpcs;
            CellRefList<ESM::Probe>             mProbes;
            CellRefList<ESM::Repair>            mRepairs;
            CellRefList<ESM::Static>            mStatics;
            CellRefList<ESM::Weapon>            mWeapons;
            CellRefList<ESM::BodyPart>          mBodyParts;

            typedef std::map<LiveCellRefBase*, MWWorld::CellStore*> MovedRefTracker;
            // References owned by a different cell that have been moved here.
            // <reference, cell the reference originally came from>
            MovedRefTracker mMovedHere;
            // References owned by this cell that have been moved to another cell.
            // <reference, cell the reference was moved to>
            MovedRefTracker mMovedToAnotherCell;

            // Merged list of ref's currently in this cell - i.e. with added refs from mMovedHere, removed refs from mMovedToAnotherCell
            std::vector<LiveCellRefBase*> mMergedRefs;

            // Get the Ptr for the given ref which originated from this cell (possibly moved to another cell at this point).
            Ptr getCurrentPtr(MWWorld::LiveCellRefBase* ref);

            /// Moves object from the given cell to this cell.
            void moveFrom(const MWWorld::Ptr& object, MWWorld::CellStore* from);

            /// Repopulate mMergedRefs.
            void updateMergedRefs();

            // (item, max charge)
            typedef std::vector<std::pair<LiveCellRefBase*, float> > TRechargingItems;
            TRechargingItems mRechargingItems;

            bool mRechargingItemsUpToDate;

            void updateRechargingItems();
            void rechargeItems(float duration);
            void checkItem(Ptr ptr);

            // helper function for forEachInternal
            template<class Visitor, class List>
            bool forEachImp (Visitor& visitor, List& list)
            {
                for (typename List::List::iterator iter (list.mList.begin()); iter!=list.mList.end();
                    ++iter)
                {
                    if (!isAccessible(iter->mData, iter->mRef))
                        continue;
                    if (!visitor (MWWorld::Ptr(&*iter, this)))
                        return false;
                }
                return true;
            }

            // listing only objects owned by this cell. Internal use only, you probably want to use forEach() so that moved objects are accounted for.
            template<class Visitor>
            bool forEachInternal (Visitor& visitor)
            {
                return
                    forEachImp (visitor, mActivators) &&
                    forEachImp (visitor, mPotions) &&
                    forEachImp (visitor, mAppas) &&
                    forEachImp (visitor, mArmors) &&
                    forEachImp (visitor, mBooks) &&
                    forEachImp (visitor, mClothes) &&
                    forEachImp (visitor, mContainers) &&
                    forEachImp (visitor, mDoors) &&
                    forEachImp (visitor, mIngreds) &&
                    forEachImp (visitor, mItemLists) &&
                    forEachImp (visitor, mLights) &&
                    forEachImp (visitor, mLockpicks) &&
                    forEachImp (visitor, mMiscItems) &&
                    forEachImp (visitor, mProbes) &&
                    forEachImp (visitor, mRepairs) &&
                    forEachImp (visitor, mStatics) &&
                    forEachImp (visitor, mWeapons) &&
                    forEachImp (visitor, mBodyParts) &&
                    forEachImp (visitor, mCreatures) &&
                    forEachImp (visitor, mNpcs) &&
                    forEachImp (visitor, mCreatureLists);
            }

            /// @note If you get a linker error here, this means the given type can not be stored in a cell. The supported types are
            /// defined at the bottom of this file.
            template <class T>
            CellRefList<T>& get();

        public:

            /// Should this reference be accessible to the outside world (i.e. to scripts / game logic)?
            /// Determined based on the deletion flags. By default, objects deleted by content files are never accessible;
            /// objects deleted by setCount(0) are still accessible *if* they came from a content file (needed for vanilla
            /// scripting compatibility, and the fact that objects may be "un-deleted" in the original game).
            static bool isAccessible(const MWWorld::RefData& refdata, const MWWorld::CellRef& cref)
            {
                return !refdata.isDeletedByContentFile() && (cref.hasContentFile() || refdata.getCount() > 0);
            }

            /// Moves object from this cell to the given cell.
            /// @note automatically updates given cell by calling cellToMoveTo->moveFrom(...)
            /// @note throws exception if cellToMoveTo == this
            /// @return updated MWWorld::Ptr with the new CellStore pointer set.
            MWWorld::Ptr moveTo(const MWWorld::Ptr& object, MWWorld::CellStore* cellToMoveTo);

            void rest(double hours);
            void recharge(float duration);

            /// Make a copy of the given object and insert it into this cell.
            /// @note If you get a linker error here, this means the given type can not be inserted into a cell.
            /// The supported types are defined at the bottom of this file.
            template <typename T>
            LiveCellRefBase* insert(const LiveCellRef<T>* ref)
            {
                mHasState = true;
                CellRefList<T>& list = get<T>();
                LiveCellRefBase* ret = &list.insert(*ref);
                updateMergedRefs();
                return ret;
            }

            /// @param readerList The readers to use for loading of the cell on-demand.
            CellStore (const ESM::Cell *cell_,
                       const MWWorld::ESMStore& store,
                       std::vector<ESM::ESMReader>& readerList);

            const ESM::Cell *getCell() const;

            State getState() const;

            const std::vector<std::string>& getPreloadedIds() const;
            ///< Get Ids of objects in this cell, only valid in State_Preloaded

            bool hasState() const;
            ///< Does this cell have state that needs to be stored in a saved game file?

            bool hasId (const std::string& id) const;
            ///< May return true for deleted IDs when in preload state. Will return false, if cell is
            /// unloaded.
            /// @note Will not account for moved references which may exist in Loaded state. Use search() instead if the cell is loaded.

            Ptr search (const std::string& id);
            ///< Will return an empty Ptr if cell is not loaded. Does not check references in
            /// containers.
            /// @note Triggers CellStore hasState flag.

            ConstPtr searchConst (const std::string& id) const;
            ///< Will return an empty Ptr if cell is not loaded. Does not check references in
            /// containers.
            /// @note Does not trigger CellStore hasState flag.

            Ptr searchViaActorId (int id);
            ///< Will return an empty Ptr if cell is not loaded.

            float getWaterLevel() const;

            bool movedHere(const MWWorld::Ptr& ptr) const;

            void setWaterLevel (float level);

            void setFog (ESM::FogState* fog);
            ///< \note Takes ownership of the pointer

            ESM::FogState* getFog () const;

            std::size_t count() const;
            ///< Return total number of references, including deleted ones.

            void load ();
            ///< Load references from content file.

            void preload ();
            ///< Build ID list from content file.

            /// Call visitor (MWWorld::Ptr) for each reference. visitor must return a bool. Returning
            /// false will abort the iteration.
            /// \note Prefer using forEachConst when possible.
            /// \note Do not modify this cell (i.e. remove/add objects) during the forEach, doing this may result in unintended behaviour.
            /// \attention This function also lists deleted (count 0) objects!
            /// \return Iteration completed?
            template<class Visitor>
            bool forEach (Visitor&& visitor)
            {
                if (mState != State_Loaded)
                    return false;

                if (mMergedRefs.empty())
                    return true;

                mHasState = true;

                for (unsigned int i=0; i<mMergedRefs.size(); ++i)
                {
                    if (!isAccessible(mMergedRefs[i]->mData, mMergedRefs[i]->mRef))
                        continue;

                    if (!visitor(MWWorld::Ptr(mMergedRefs[i], this)))
                        return false;
                }
                return true;
            }

            /// Call visitor (MWWorld::ConstPtr) for each reference. visitor must return a bool. Returning
            /// false will abort the iteration.
            /// \note Do not modify this cell (i.e. remove/add objects) during the forEach, doing this may result in unintended behaviour.
            /// \attention This function also lists deleted (count 0) objects!
            /// \return Iteration completed?
            template<class Visitor>
            bool forEachConst (Visitor&& visitor) const
            {
                if (mState != State_Loaded)
                    return false;

                for (unsigned int i=0; i<mMergedRefs.size(); ++i)
                {
                    if (!isAccessible(mMergedRefs[i]->mData, mMergedRefs[i]->mRef))
                        continue;

                    if (!visitor(MWWorld::ConstPtr(mMergedRefs[i], this)))
                        return false;
                }
                return true;
            }


            /// Call visitor (ref) for each reference of given type. visitor must return a bool. Returning
            /// false will abort the iteration.
            /// \note Do not modify this cell (i.e. remove/add objects) during the forEach, doing this may result in unintended behaviour.
            /// \attention This function also lists deleted (count 0) objects!
            /// \return Iteration completed?
            template <class T, class Visitor>
            bool forEachType(Visitor& visitor)
            {
                if (mState != State_Loaded)
                    return false;

                if (mMergedRefs.empty())
                    return true;

                mHasState = true;

                CellRefList<T>& list = get<T>();

                for (typename CellRefList<T>::List::iterator it (list.mList.begin()); it!=list.mList.end(); ++it)
                {
                    LiveCellRefBase* base = &*it;
                    if (mMovedToAnotherCell.find(base) != mMovedToAnotherCell.end())
                        continue;
                    if (!isAccessible(base->mData, base->mRef))
                        continue;
                    if (!visitor(MWWorld::Ptr(base, this)))
                        return false;
                }

                for (MovedRefTracker::const_iterator it = mMovedHere.begin(); it != mMovedHere.end(); ++it)
                {
                    LiveCellRefBase* base = it->first;
                    if (dynamic_cast<LiveCellRef<T>*>(base))
                        if (!visitor(MWWorld::Ptr(base, this)))
                            return false;
                }
                return true;
            }

            // NOTE: does not account for moved references
            // Should be phased out when we have const version of forEach
            inline const CellRefList<ESM::Door>& getReadOnlyDoors() const
            {
                return mDoors;
            }
            inline const CellRefList<ESM::Static>& getReadOnlyStatics() const
            {
                return mStatics;
            }

            bool isExterior() const;

            Ptr searchInContainer (const std::string& id);

            void loadState (const ESM::CellState& state);

            void saveState (ESM::CellState& state) const;

            void writeFog (ESM::ESMWriter& writer) const;

            void readFog (ESM::ESMReader& reader);

            void writeReferences (ESM::ESMWriter& writer) const;

            struct GetCellStoreCallback
            {
            public:
                ///@note must return nullptr if the cell is not found
                virtual CellStore* getCellStore(const ESM::CellId& cellId) = 0;
                virtual ~GetCellStoreCallback() = default;
            };

            /// @param callback to use for retrieving of additional CellStore objects by ID (required for resolving moved references)
            void readReferences (ESM::ESMReader& reader, const std::map<int, int>& contentFileMap, GetCellStoreCallback* callback);

            void respawn ();
            ///< Check mLastRespawn and respawn references if necessary. This is a no-op if the cell is not loaded.

        private:

            /// Run through references and store IDs
            void listRefs();

            void loadRefs();

            void loadRef (ESM::CellRef& ref, bool deleted, std::map<ESM::RefNum, std::string>& refNumToID);
            ///< Make case-adjustments to \a ref and insert it into the respective container.
            ///
            /// Invalid \a ref objects are silently dropped.
    };

    template<>
    inline CellRefList<ESM::Activator>& CellStore::get<ESM::Activator>()
    {
        mHasState = true;
        return mActivators;
    }

    template<>
    inline CellRefList<ESM::Potion>& CellStore::get<ESM::Potion>()
    {
        mHasState = true;
        return mPotions;
    }

    template<>
    inline CellRefList<ESM::Apparatus>& CellStore::get<ESM::Apparatus>()
    {
        mHasState = true;
        return mAppas;
    }

    template<>
    inline CellRefList<ESM::Armor>& CellStore::get<ESM::Armor>()
    {
        mHasState = true;
        return mArmors;
    }

    template<>
    inline CellRefList<ESM::Book>& CellStore::get<ESM::Book>()
    {
        mHasState = true;
        return mBooks;
    }

    template<>
    inline CellRefList<ESM::Clothing>& CellStore::get<ESM::Clothing>()
    {
        mHasState = true;
        return mClothes;
    }

    template<>
    inline CellRefList<ESM::Container>& CellStore::get<ESM::Container>()
    {
        mHasState = true;
        return mContainers;
    }

    template<>
    inline CellRefList<ESM::Creature>& CellStore::get<ESM::Creature>()
    {
        mHasState = true;
        return mCreatures;
    }

    template<>
    inline CellRefList<ESM::Door>& CellStore::get<ESM::Door>()
    {
        mHasState = true;
        return mDoors;
    }

    template<>
    inline CellRefList<ESM::Ingredient>& CellStore::get<ESM::Ingredient>()
    {
        mHasState = true;
        return mIngreds;
    }

    template<>
    inline CellRefList<ESM::CreatureLevList>& CellStore::get<ESM::CreatureLevList>()
    {
        mHasState = true;
        return mCreatureLists;
    }

    template<>
    inline CellRefList<ESM::ItemLevList>& CellStore::get<ESM::ItemLevList>()
    {
        mHasState = true;
        return mItemLists;
    }

    template<>
    inline CellRefList<ESM::Light>& CellStore::get<ESM::Light>()
    {
        mHasState = true;
        return mLights;
    }

    template<>
    inline CellRefList<ESM::Lockpick>& CellStore::get<ESM::Lockpick>()
    {
        mHasState = true;
        return mLockpicks;
    }

    template<>
    inline CellRefList<ESM::Miscellaneous>& CellStore::get<ESM::Miscellaneous>()
    {
        mHasState = true;
        return mMiscItems;
    }

    template<>
    inline CellRefList<ESM::NPC>& CellStore::get<ESM::NPC>()
    {
        mHasState = true;
        return mNpcs;
    }

    template<>
    inline CellRefList<ESM::Probe>& CellStore::get<ESM::Probe>()
    {
        mHasState = true;
        return mProbes;
    }

    template<>
    inline CellRefList<ESM::Repair>& CellStore::get<ESM::Repair>()
    {
        mHasState = true;
        return mRepairs;
    }

    template<>
    inline CellRefList<ESM::Static>& CellStore::get<ESM::Static>()
    {
        mHasState = true;
        return mStatics;
    }

    template<>
    inline CellRefList<ESM::Weapon>& CellStore::get<ESM::Weapon>()
    {
        mHasState = true;
        return mWeapons;
    }

    template<>
    inline CellRefList<ESM::BodyPart>& CellStore::get<ESM::BodyPart>()
    {
        mHasState = true;
        return mBodyParts;
    }

    bool operator== (const CellStore& left, const CellStore& right);
    bool operator!= (const CellStore& left, const CellStore& right);
}

#endif
