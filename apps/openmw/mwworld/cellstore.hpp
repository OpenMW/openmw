#ifndef GAME_MWWORLD_CELLSTORE_H
#define GAME_MWWORLD_CELLSTORE_H

#include <algorithm>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <map>

#include <boost/shared_ptr.hpp>

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

#include "../mwmechanics/pathgrid.hpp"  // TODO: maybe belongs in mwworld

#include "timestamp.hpp"

namespace ESM
{
    struct CellState;
    struct FogState;
    struct CellId;
}

namespace MWWorld
{
    class Ptr;
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
            // Note this is NULL until the cell is explored to save some memory
            boost::shared_ptr<ESM::FogState> mFogState;

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

            typedef std::map<LiveCellRefBase*, MWWorld::CellStore*> MovedRefTracker;
            // References owned by a different cell that have been moved here.
            // <reference, cell the reference originally came from>
            MovedRefTracker mMovedHere;
            // References owned by this cell that have been moved to another cell.
            // <reference, cell the reference was moved to>
            MovedRefTracker mMovedToAnotherCell;

            // Merged list of ref's currently in this cell - i.e. with added refs from mMovedHere, removed refs from mMovedToAnotherCell
            std::vector<LiveCellRefBase*> mMergedRefs;

            /// Moves object from the given cell to this cell.
            void moveFrom(const MWWorld::Ptr& object, MWWorld::CellStore* from);

            /// Repopulate mMergedRefs.
            void updateMergedRefs();

            template<typename T>
            LiveCellRefBase* insertBase(CellRefList<T>& list, const LiveCellRef<T>* ref)
            {
                mHasState = true;
                LiveCellRefBase* ret = &list.insert(*ref);
                updateMergedRefs();
                return ret;
            }

            // helper function for forEachInternal
            template<class Visitor, class List>
            bool forEachImp (Visitor& visitor, List& list)
            {
                for (typename List::List::iterator iter (list.mList.begin()); iter!=list.mList.end();
                    ++iter)
                {
                    if (iter->mData.isDeletedByContentFile())
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
                    forEachImp (visitor, mCreatures) &&
                    forEachImp (visitor, mNpcs) &&
                    forEachImp (visitor, mCreatureLists);
            }

        public:

            /// Moves object from this cell to the given cell.
            /// @note automatically updates given cell by calling cellToMoveTo->moveFrom(...)
            /// @note throws exception if cellToMoveTo == this
            /// @return updated MWWorld::Ptr with the new CellStore pointer set.
            MWWorld::Ptr moveTo(const MWWorld::Ptr& object, MWWorld::CellStore* cellToMoveTo);

            /// Make a copy of the given object and insert it into this cell.
            /// @note If you get a linker error here, this means the given type can not be inserted into a cell.
            /// The supported types are defined at the bottom of this file.
            template <typename T>
            LiveCellRefBase* insert(const LiveCellRef<T>* ref);

            /// @param readerList The readers to use for loading of the cell on-demand.
            CellStore (const ESM::Cell *cell_,
                       const MWWorld::ESMStore& store,
                       std::vector<ESM::ESMReader>& readerList);

            const ESM::Cell *getCell() const;

            State getState() const;

            bool hasState() const;
            ///< Does this cell have state that needs to be stored in a saved game file?

            bool hasId (const std::string& id) const;
            ///< May return true for deleted IDs when in preload state. Will return false, if cell is
            /// unloaded.
            /// @note Will not account for moved references which may exist in Loaded state. Use search() instead if the cell is loaded.

            Ptr search (const std::string& id);
            ///< Will return an empty Ptr if cell is not loaded. Does not check references in
            /// containers.

            Ptr searchViaActorId (int id);
            ///< Will return an empty Ptr if cell is not loaded.

            float getWaterLevel() const;

            void setWaterLevel (float level);

            void setFog (ESM::FogState* fog);
            ///< \note Takes ownership of the pointer

            ESM::FogState* getFog () const;

            int count() const;
            ///< Return total number of references, including deleted ones.

            void load ();
            ///< Load references from content file.

            void preload ();
            ///< Build ID list from content file.

            /// Call visitor (ref) for each reference. visitor must return a bool. Returning
            /// false will abort the iteration.
            /// \attention This function also lists deleted (count 0) objects!
            /// \return Iteration completed?
            template<class Visitor>
            bool forEach (Visitor& visitor)
            {
                if (mState != State_Loaded)
                    return false;

                mHasState = true;

                for (unsigned int i=0; i<mMergedRefs.size(); ++i)
                {
                    if (mMergedRefs[i]->mData.isDeletedByContentFile())
                        continue;

                    if (!visitor(MWWorld::Ptr(mMergedRefs[i], this)))
                        return false;
                }
                return true;
            }

            /// \todo add const version of forEach

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
                ///@note must return NULL if the cell is not found
                virtual CellStore* getCellStore(const ESM::CellId& cellId) = 0;
            };

            /// @param callback to use for retrieving of additional CellStore objects by ID (required for resolving moved references)
            void readReferences (ESM::ESMReader& reader, const std::map<int, int>& contentFileMap, GetCellStoreCallback* callback);

            void respawn ();
            ///< Check mLastRespawn and respawn references if necessary. This is a no-op if the cell is not loaded.

            bool isPointConnected(const int start, const int end) const;

            std::list<ESM::Pathgrid::Point> aStarSearch(const int start, const int end) const;

        private:

            /// Run through references and store IDs
            void listRefs();

            void loadRefs();

            void loadRef (ESM::CellRef& ref, bool deleted);
            ///< Make case-adjustments to \a ref and insert it into the respective container.
            ///
            /// Invalid \a ref objects are silently dropped.

            MWMechanics::PathgridGraph mPathgridGraph;
    };

    template<>
    inline LiveCellRefBase* CellStore::insert<ESM::Activator>(const LiveCellRef<ESM::Activator>* ref)
    {
        return insertBase(mActivators, ref);
    }
    template<>
    inline LiveCellRefBase* CellStore::insert<ESM::Potion>(const LiveCellRef<ESM::Potion>* ref)
    {
        return insertBase(mPotions, ref);
    }
    template<>
    inline LiveCellRefBase* CellStore::insert<ESM::Apparatus>(const LiveCellRef<ESM::Apparatus>* ref)
    {
        return insertBase(mAppas, ref);
    }
    template<>
    inline LiveCellRefBase* CellStore::insert<ESM::Armor>(const LiveCellRef<ESM::Armor>* ref)
    {
        return insertBase(mArmors, ref);
    }
    template<>
    inline LiveCellRefBase* CellStore::insert<ESM::Book>(const LiveCellRef<ESM::Book>* ref)
    {
        return insertBase(mBooks, ref);
    }
    template<>
    inline LiveCellRefBase* CellStore::insert<ESM::Clothing>(const LiveCellRef<ESM::Clothing>* ref)
    {
        return insertBase(mClothes, ref);
    }
    template<>
    inline LiveCellRefBase* CellStore::insert<ESM::Container>(const LiveCellRef<ESM::Container>* ref)
    {
        return insertBase(mContainers, ref);
    }
    template<>
    inline LiveCellRefBase* CellStore::insert<ESM::Creature>(const LiveCellRef<ESM::Creature>* ref)
    {
        return insertBase(mCreatures, ref);
    }
    template<>
    inline LiveCellRefBase* CellStore::insert<ESM::Door>(const LiveCellRef<ESM::Door>* ref)
    {
        return insertBase(mDoors, ref);
    }
    template<>
    inline LiveCellRefBase* CellStore::insert<ESM::Ingredient>(const LiveCellRef<ESM::Ingredient>* ref)
    {
        return insertBase(mIngreds, ref);
    }
    template<>
    inline LiveCellRefBase* CellStore::insert<ESM::CreatureLevList>(const LiveCellRef<ESM::CreatureLevList>* ref)
    {
        return insertBase(mCreatureLists, ref);
    }
    template<>
    inline LiveCellRefBase* CellStore::insert<ESM::ItemLevList>(const LiveCellRef<ESM::ItemLevList>* ref)
    {
        return insertBase(mItemLists, ref);
    }
    template<>
    inline LiveCellRefBase* CellStore::insert<ESM::Light>(const LiveCellRef<ESM::Light>* ref)
    {
        return insertBase(mLights, ref);
    }
    template<>
    inline LiveCellRefBase* CellStore::insert<ESM::Lockpick>(const LiveCellRef<ESM::Lockpick>* ref)
    {
        return insertBase(mLockpicks, ref);
    }
    template<>
    inline LiveCellRefBase* CellStore::insert<ESM::Miscellaneous>(const LiveCellRef<ESM::Miscellaneous>* ref)
    {
        return insertBase(mMiscItems, ref);
    }
    template<>
    inline LiveCellRefBase* CellStore::insert<ESM::NPC>(const LiveCellRef<ESM::NPC>* ref)
    {
        return insertBase(mNpcs, ref);
    }
    template<>
    inline LiveCellRefBase* CellStore::insert<ESM::Probe>(const LiveCellRef<ESM::Probe>* ref)
    {
        return insertBase(mProbes, ref);
    }
    template<>
    inline LiveCellRefBase* CellStore::insert<ESM::Repair>(const LiveCellRef<ESM::Repair>* ref)
    {
        return insertBase(mRepairs, ref);
    }
    template<>
    inline LiveCellRefBase* CellStore::insert<ESM::Static>(const LiveCellRef<ESM::Static>* ref)
    {
        return insertBase(mStatics, ref);
    }
    template<>
    inline LiveCellRefBase* CellStore::insert<ESM::Weapon>(const LiveCellRef<ESM::Weapon>* ref)
    {
        return insertBase(mWeapons, ref);
    }

    bool operator== (const CellStore& left, const CellStore& right);
    bool operator!= (const CellStore& left, const CellStore& right);
}

#endif
