#include "cellstore.hpp"

#include <iostream>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "ptr.hpp"
#include "esmstore.hpp"

namespace MWWorld
{
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

        if (mCell->mContextList.size() == 0)
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
                std::string lowerCase;

                std::transform (ref.mRefID.begin(), ref.mRefID.end(), std::back_inserter (lowerCase),
                    (int(*)(int)) std::tolower);

                // TODO: Fully support deletion of references.
                mIds.push_back (lowerCase);
            }
        }

        std::sort (mIds.begin(), mIds.end());
    }

    void CellStore::loadRefs(const MWWorld::ESMStore &store, std::vector<ESM::ESMReader> &esm)
    {
      assert (mCell);

        if (mCell->mContextList.size() == 0)
            return; // this is a dynamically generated cell -> skipping.

        // Load references from all plugins that do something with this cell.
        for (size_t i = 0; i < mCell->mContextList.size(); i++)
        {
            if (mCell->mContextList.size() > 1)
                std::cout << "number of lists " << mCell->mContextList.size() << std::endl;
            // Reopen the ESM reader and seek to the right position.
            int index = mCell->mContextList.at(i).index;
            mCell->restore (esm[index], i);

            ESM::CellRef ref;

            // Get each reference in turn
            while(mCell->getNextRef(esm[index], ref))
            {
                std::string lowerCase;

                std::transform (ref.mRefID.begin(), ref.mRefID.end(), std::back_inserter (lowerCase),
                    (int(*)(int)) std::tolower);

                // TODO: Fully support deletion of references.
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
    }
}
