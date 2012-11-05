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

    void CellStore::load (const MWWorld::ESMStore &store, ESM::ESMReader &esm)
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

    void CellStore::preload (const MWWorld::ESMStore &store, ESM::ESMReader &esm)
    {
        if (mState==State_Unloaded)
        {
            listRefs (store, esm);

            mState = State_Preloaded;
        }
    }

    void CellStore::listRefs(const MWWorld::ESMStore &store, ESM::ESMReader &esm)
    {
        assert (mCell);

        if (mCell->mContext.filename.empty())
            return; // this is a dynamically generated cell -> skipping.

        // Reopen the ESM reader and seek to the right position.
        mCell->restore (esm);

        ESM::CellRef ref;

        // Get each reference in turn
        while (mCell->getNextRef (esm, ref))
        {
            std::string lowerCase;

            std::transform (ref.mRefID.begin(), ref.mRefID.end(), std::back_inserter (lowerCase),
                (int(*)(int)) std::tolower);

            mIds.push_back (lowerCase);
        }

        std::sort (mIds.begin(), mIds.end());
    }

    void CellStore::loadRefs(const MWWorld::ESMStore &store, ESM::ESMReader &esm)
    {
      assert (mCell);

        if (mCell->mContext.filename.empty())
            return; // this is a dynamically generated cell -> skipping.

      // Reopen the ESM reader and seek to the right position.
      mCell->restore(esm);

      ESM::CellRef ref;

      // Get each reference in turn
      while(mCell->getNextRef(esm, ref))
        {
            std::string lowerCase;

            std::transform (ref.mRefID.begin(), ref.mRefID.end(), std::back_inserter (lowerCase),
                (int(*)(int)) std::tolower);

            int rec = store.find(ref.mRefID);

            ref.mRefID = lowerCase;

          /* We can optimize this further by storing the pointer to the
             record itself in store.all, so that we don't need to look it
             up again here. However, never optimize. There are infinite
             opportunities to do that later.
           */
          switch(rec)
            {
            case ESM::REC_ACTI: mActivators.find(ref, store.activators); break;
            case ESM::REC_ALCH: mPotions.find(ref, store.potions); break;
            case ESM::REC_APPA: mAppas.find(ref, store.appas); break;
            case ESM::REC_ARMO: mArmors.find(ref, store.armors); break;
            case ESM::REC_BOOK: mBooks.find(ref, store.books); break;
            case ESM::REC_CLOT: mClothes.find(ref, store.clothes); break;
            case ESM::REC_CONT: mContainers.find(ref, store.containers); break;
            case ESM::REC_CREA: mCreatures.find(ref, store.creatures); break;
            case ESM::REC_DOOR: mDoors.find(ref, store.doors); break;
            case ESM::REC_INGR: mIngreds.find(ref, store.ingreds); break;
            case ESM::REC_LEVC: mCreatureLists.find(ref, store.creatureLists); break;
            case ESM::REC_LEVI: mItemLists.find(ref, store.itemLists); break;
            case ESM::REC_LIGH: mLights.find(ref, store.lights); break;
            case ESM::REC_LOCK: mLockpicks.find(ref, store.lockpicks); break;
            case ESM::REC_MISC: mMiscItems.find(ref, store.miscItems); break;
            case ESM::REC_NPC_: mNpcs.find(ref, store.npcs); break;
            case ESM::REC_PROB: mProbes.find(ref, store.probes); break;
            case ESM::REC_REPA: mRepairs.find(ref, store.repairs); break;
            case ESM::REC_STAT: mStatics.find(ref, store.statics); break;
            case ESM::REC_WEAP: mWeapons.find(ref, store.weapons); break;

            case 0: std::cout << "Cell reference " + ref.mRefID + " not found!\n"; break;
            default:
              std::cout << "WARNING: Ignoring reference '" << ref.mRefID << "' of unhandled type\n";
            }
        }
    }
}
