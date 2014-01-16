#include "cellstore.hpp"

#include <iostream>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "ptr.hpp"
#include "esmstore.hpp"
#include "class.hpp"
#include "containerstore.hpp"

namespace
{
    template<typename T>
    MWWorld::Ptr searchInContainerList (MWWorld::CellRefList<T>& containerList, const std::string& id)
    {
        for (typename MWWorld::CellRefList<T>::List::iterator iter (containerList.mList.begin());
             iter!=containerList.mList.end(); ++iter)
        {
            MWWorld::Ptr container (&*iter, 0);

            MWWorld::Ptr ptr =
                MWWorld::Class::get (container).getContainerStore (container).search (id);

            if (!ptr.isEmpty())
                return ptr;
        }

        return MWWorld::Ptr();
    }
}

namespace MWWorld
{

    template <typename X>
    void CellRefList<X>::load(ESM::CellRef &ref, const MWWorld::ESMStore &esmStore)
    {
        // Get existing reference, in case we need to overwrite it.
        typename std::list<LiveRef>::iterator iter = std::find(mList.begin(), mList.end(), ref.mRefnum);

        // Skip this when reference was deleted.
        // TODO: Support respawning references, in this case, we need to track it somehow.
        if (ref.mDeleted) {
            if (iter != mList.end())
                mList.erase(iter);
            return;
        }

        // for throwing exception on unhandled record type
        const MWWorld::Store<X> &store = esmStore.get<X>();
        const X *ptr = store.search(ref.mRefID);

        /// \note no longer redundant - changed to Store<X>::search(), don't throw
        ///  an exception on miss, try to continue (that's how MW does it, anyway)
        if (ptr == NULL) {
            std::cout << "Warning: could not resolve cell reference " << ref.mRefID << ", trying to continue anyway" << std::endl;
        } else {
          if (iter != mList.end())
            *iter = LiveRef(ref, ptr);
          else
            mList.push_back(LiveRef(ref, ptr));
        }
    }

    template<typename X> bool operator==(const LiveCellRef<X>& ref, int pRefnum)
    {
        return (ref.mRef.mRefnum == pRefnum);
    }

    CellStore::CellStore (const ESM::Cell *cell)
      : mCell (cell), mState (State_Unloaded)
    {
        mWaterLevel = cell->mWater;
    }

    void CellStore::load (const MWWorld::ESMStore &store, std::vector<ESM::ESMReader> &esm)
    {
        if (mState!=State_Loaded)
        {
            if (mState==State_Preloaded)
                mIds.clear();

            std::cout << "loading cell " << mCell->getDescription() << std::endl;

            loadRefs (store, esm);

            mState = State_Loaded;
        }
    }

    void CellStore::preload (const MWWorld::ESMStore &store, std::vector<ESM::ESMReader> &esm)
    {
        if (mState==State_Unloaded)
        {
            listRefs (store, esm);

            mState = State_Preloaded;
        }
    }

    void CellStore::listRefs(const MWWorld::ESMStore &store, std::vector<ESM::ESMReader> &esm)
    {
        assert (mCell);

        if (mCell->mContextList.empty())
            return; // this is a dynamically generated cell -> skipping.

        // Load references from all plugins that do something with this cell.
        for (size_t i = 0; i < mCell->mContextList.size(); i++)
        {
            // Reopen the ESM reader and seek to the right position.
            int index = mCell->mContextList.at(i).index;
            mCell->restore (esm[index], i);

            ESM::CellRef ref;

            // Get each reference in turn
            while (mCell->getNextRef (esm[index], ref))
            {
                std::string lowerCase = Misc::StringUtils::lowerCase (ref.mRefID);
                if (ref.mDeleted) {
                    // Right now, don't do anything. Where is "listRefs" actually used, anyway?
                    //  Skipping for now...
                    continue;
                }

                mIds.push_back (lowerCase);
            }
        }

        std::sort (mIds.begin(), mIds.end());
    }

    void CellStore::loadRefs(const MWWorld::ESMStore &store, std::vector<ESM::ESMReader> &esm)
    {
      assert (mCell);

        if (mCell->mContextList.empty())
            return; // this is a dynamically generated cell -> skipping.

        // Load references from all plugins that do something with this cell.
        for (size_t i = 0; i < mCell->mContextList.size(); i++)
        {
            // Reopen the ESM reader and seek to the right position.
            int index = mCell->mContextList.at(i).index;
            mCell->restore (esm[index], i);

            ESM::CellRef ref;

            // Get each reference in turn
            while(mCell->getNextRef(esm[index], ref))
            {
                // Don't load reference if it was moved to a different cell.
                std::string lowerCase = Misc::StringUtils::lowerCase(ref.mRefID);
                ESM::MovedCellRefTracker::const_iterator iter = std::find(mCell->mMovedRefs.begin(), mCell->mMovedRefs.end(), ref.mRefnum);
                if (iter != mCell->mMovedRefs.end()) {
                    continue;
                }
                int rec = store.find(ref.mRefID);

                ref.mRefID = lowerCase;

            /* We can optimize this further by storing the pointer to the
                record itself in store.all, so that we don't need to look it
                up again here. However, never optimize. There are infinite
                opportunities to do that later.
            */
            switch(rec)
                {
            case ESM::REC_ACTI: mActivators.load(ref, store); break;
            case ESM::REC_ALCH: mPotions.load(ref, store); break;
            case ESM::REC_APPA: mAppas.load(ref, store); break;
            case ESM::REC_ARMO: mArmors.load(ref, store); break;
            case ESM::REC_BOOK: mBooks.load(ref, store); break;
            case ESM::REC_CLOT: mClothes.load(ref, store); break;
            case ESM::REC_CONT: mContainers.load(ref, store); break;
            case ESM::REC_CREA: mCreatures.load(ref, store); break;
            case ESM::REC_DOOR: mDoors.load(ref, store); break;
            case ESM::REC_INGR: mIngreds.load(ref, store); break;
            case ESM::REC_LEVC: mCreatureLists.load(ref, store); break;
            case ESM::REC_LEVI: mItemLists.load(ref, store); break;
            case ESM::REC_LIGH: mLights.load(ref, store); break;
            case ESM::REC_LOCK: mLockpicks.load(ref, store); break;
            case ESM::REC_MISC: mMiscItems.load(ref, store); break;
            case ESM::REC_NPC_: mNpcs.load(ref, store); break;
            case ESM::REC_PROB: mProbes.load(ref, store); break;
            case ESM::REC_REPA: mRepairs.load(ref, store); break;
            case ESM::REC_STAT: mStatics.load(ref, store); break;
            case ESM::REC_WEAP: mWeapons.load(ref, store); break;

                case 0: std::cout << "Cell reference " + ref.mRefID + " not found!\n"; break;
                default:
                std::cout << "WARNING: Ignoring reference '" << ref.mRefID << "' of unhandled type\n";
                }
            }
        }

        // Load moved references, from separately tracked list.
        for (ESM::CellRefTracker::const_iterator it = mCell->mLeasedRefs.begin(); it != mCell->mLeasedRefs.end(); ++it)
        {
            // Doesn't seem to work in one line... huh? Too sleepy to check...
            ESM::CellRef &ref = const_cast<ESM::CellRef&>(*it);
            //ESM::CellRef &ref = const_cast<ESM::CellRef&>(it->second);

            int rec = store.find(ref.mRefID);
            Misc::StringUtils::toLower(ref.mRefID);

            /* We can optimize this further by storing the pointer to the
                record itself in store.all, so that we don't need to look it
                up again here. However, never optimize. There are infinite
                opportunities to do that later.
            */
            switch(rec)
                {
            case ESM::REC_ACTI: mActivators.load(ref, store); break;
            case ESM::REC_ALCH: mPotions.load(ref, store); break;
            case ESM::REC_APPA: mAppas.load(ref, store); break;
            case ESM::REC_ARMO: mArmors.load(ref, store); break;
            case ESM::REC_BOOK: mBooks.load(ref, store); break;
            case ESM::REC_CLOT: mClothes.load(ref, store); break;
            case ESM::REC_CONT: mContainers.load(ref, store); break;
            case ESM::REC_CREA: mCreatures.load(ref, store); break;
            case ESM::REC_DOOR: mDoors.load(ref, store); break;
            case ESM::REC_INGR: mIngreds.load(ref, store); break;
            case ESM::REC_LEVC: mCreatureLists.load(ref, store); break;
            case ESM::REC_LEVI: mItemLists.load(ref, store); break;
            case ESM::REC_LIGH: mLights.load(ref, store); break;
            case ESM::REC_LOCK: mLockpicks.load(ref, store); break;
            case ESM::REC_MISC: mMiscItems.load(ref, store); break;
            case ESM::REC_NPC_: mNpcs.load(ref, store); break;
            case ESM::REC_PROB: mProbes.load(ref, store); break;
            case ESM::REC_REPA: mRepairs.load(ref, store); break;
            case ESM::REC_STAT: mStatics.load(ref, store); break;
            case ESM::REC_WEAP: mWeapons.load(ref, store); break;

                case 0: std::cout << "Cell reference " + ref.mRefID + " not found!\n"; break;
                default:
                std::cout << "WARNING: Ignoring reference '" << ref.mRefID << "' of unhandled type\n";
                }

        }
    }

    Ptr CellStore::searchInContainer (const std::string& id)
    {
        {
            Ptr ptr = searchInContainerList (mContainers, id);

            if (!ptr.isEmpty())
                return ptr;
        }

        {
            Ptr ptr = searchInContainerList (mCreatures, id);

            if (!ptr.isEmpty())
                return ptr;
        }

        {
            Ptr ptr = searchInContainerList (mNpcs, id);

            if (!ptr.isEmpty())
                return ptr;
        }

        return Ptr();
    }
}
