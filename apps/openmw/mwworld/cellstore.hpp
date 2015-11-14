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

        public:

            CellStore (const ESM::Cell *cell_);

            const ESM::Cell *getCell() const;

            State getState() const;

            bool hasState() const;
            ///< Does this cell have state that needs to be stored in a saved game file?

            bool hasId (const std::string& id) const;
            ///< May return true for deleted IDs when in preload state. Will return false, if cell is
            /// unloaded.

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

            void load (const MWWorld::ESMStore &store, std::vector<ESM::ESMReader> &esm);
            ///< Load references from content file.

            void preload (const MWWorld::ESMStore &store, std::vector<ESM::ESMReader> &esm);
            ///< Build ID list from content file.

            /// Call functor (ref) for each reference. functor must return a bool. Returning
            /// false will abort the iteration.
            /// \attention This function also lists deleted (count 0) objects!
            /// \return Iteration completed?
            ///
            /// \note Creatures and NPCs are handled last.
            template<class Functor>
            bool forEach (Functor& functor)
            {
                mHasState = true;

                return
                    forEachImp (functor, mActivators) &&
                    forEachImp (functor, mPotions) &&
                    forEachImp (functor, mAppas) &&
                    forEachImp (functor, mArmors) &&
                    forEachImp (functor, mBooks) &&
                    forEachImp (functor, mClothes) &&
                    forEachImp (functor, mContainers) &&
                    forEachImp (functor, mDoors) &&
                    forEachImp (functor, mIngreds) &&
                    forEachImp (functor, mItemLists) &&
                    forEachImp (functor, mLights) &&
                    forEachImp (functor, mLockpicks) &&
                    forEachImp (functor, mMiscItems) &&
                    forEachImp (functor, mProbes) &&
                    forEachImp (functor, mRepairs) &&
                    forEachImp (functor, mStatics) &&
                    forEachImp (functor, mWeapons) &&
                    forEachImp (functor, mCreatures) &&
                    forEachImp (functor, mNpcs) &&
                    forEachImp (functor, mCreatureLists);
            }

            template<class Functor>
            bool forEachContainer (Functor& functor)
            {
                mHasState = true;

                return
                    forEachImp (functor, mContainers) &&
                    forEachImp (functor, mCreatures) &&
                    forEachImp (functor, mNpcs);
            }

            /// \todo add const version of forEach

            bool isExterior() const;

            Ptr searchInContainer (const std::string& id);

            void loadState (const ESM::CellState& state);

            void saveState (ESM::CellState& state) const;

            void writeFog (ESM::ESMWriter& writer) const;

            void readFog (ESM::ESMReader& reader);

            void writeReferences (ESM::ESMWriter& writer) const;

            void readReferences (ESM::ESMReader& reader, const std::map<int, int>& contentFileMap);

            void respawn ();
            ///< Check mLastRespawn and respawn references if necessary. This is a no-op if the cell is not loaded.

            bool isPointConnected(const int start, const int end) const;

            std::list<ESM::Pathgrid::Point> aStarSearch(const int start, const int end) const;

        private:

            template<class Functor, class List>
            bool forEachImp (Functor& functor, List& list)
            {
                for (typename List::List::iterator iter (list.mList.begin()); iter!=list.mList.end();
                    ++iter)
                {
                    if (iter->mData.isDeletedByContentFile())
                        continue;
                    if (!functor (MWWorld::Ptr(&*iter, this)))
                        return false;
                }
                return true;
            }

            /// Run through references and store IDs
            void listRefs(const MWWorld::ESMStore &store, std::vector<ESM::ESMReader> &esm);

            void loadRefs(const MWWorld::ESMStore &store, std::vector<ESM::ESMReader> &esm);

            void loadRef (ESM::CellRef& ref, bool deleted, const ESMStore& store);
            ///< Make case-adjustments to \a ref and insert it into the respective container.
            ///
            /// Invalid \a ref objects are silently dropped.

            MWMechanics::PathgridGraph mPathgridGraph;
    };

    bool operator== (const CellStore& left, const CellStore& right);
    bool operator!= (const CellStore& left, const CellStore& right);
}

#endif
