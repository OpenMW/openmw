#include "cells.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "class.hpp"
#include "esmstore.hpp"
#include "containerstore.hpp"

MWWorld::CellStore *MWWorld::Cells::getCellStore (const ESM::Cell *cell)
{
    if (cell->mData.mFlags & ESM::Cell::Interior)
    {
        std::map<std::string, CellStore>::iterator result = mInteriors.find (Misc::StringUtils::lowerCase(cell->mName));

        if (result==mInteriors.end())
        {
            result = mInteriors.insert (std::make_pair (Misc::StringUtils::lowerCase(cell->mName), CellStore (cell))).first;
        }

        return &result->second;
    }
    else
    {
        std::map<std::pair<int, int>, CellStore>::iterator result =
            mExteriors.find (std::make_pair (cell->getGridX(), cell->getGridY()));

        if (result==mExteriors.end())
        {
            result = mExteriors.insert (std::make_pair (
                std::make_pair (cell->getGridX(), cell->getGridY()), CellStore (cell))).first;

        }

        return &result->second;
    }
}

void MWWorld::Cells::clear()
{
    mInteriors.clear();
    mExteriors.clear();
    std::fill(mIdCache.begin(), mIdCache.end(), std::make_pair("", (MWWorld::CellStore*)0));
    mIdCacheIndex = 0;
}

MWWorld::Ptr MWWorld::Cells::getPtrAndCache (const std::string& name, CellStore& cellStore)
{
    Ptr ptr = getPtr (name, cellStore);

    if (!ptr.isEmpty() && ptr.isInCell())
    {
        mIdCache[mIdCacheIndex].first = name;
        mIdCache[mIdCacheIndex].second = &cellStore;
        if (++mIdCacheIndex>=mIdCache.size())
            mIdCacheIndex = 0;
    }

    return ptr;
}

MWWorld::Cells::Cells (const MWWorld::ESMStore& store, std::vector<ESM::ESMReader>& reader)
: mStore (store), mReader (reader),
  mIdCache (40, std::pair<std::string, CellStore *> ("", (CellStore*)0)), /// \todo make cache size configurable
  mIdCacheIndex (0)
{}

MWWorld::CellStore *MWWorld::Cells::getExterior (int x, int y)
{
    std::map<std::pair<int, int>, CellStore>::iterator result =
        mExteriors.find (std::make_pair (x, y));

    if (result==mExteriors.end())
    {
        const ESM::Cell *cell = mStore.get<ESM::Cell>().search(x, y);

        if (!cell)
        {
            // Cell isn't predefined. Make one on the fly.
            ESM::Cell record;

            record.mData.mFlags = 0;
            record.mData.mX = x;
            record.mData.mY = y;
            record.mWater = 0;
            record.mMapColor = 0;

            cell = MWBase::Environment::get().getWorld()->createRecord (record);
        }

        result = mExteriors.insert (std::make_pair (
            std::make_pair (x, y), CellStore (cell))).first;
    }

    if (result->second.mState!=CellStore::State_Loaded)
    {
        // Multiple plugin support for landscape data is much easier than for references. The last plugin wins.
        result->second.load (mStore, mReader);
    }

    return &result->second;
}

MWWorld::CellStore *MWWorld::Cells::getInterior (const std::string& name)
{
    std::string lowerName = Misc::StringUtils::lowerCase(name);
    std::map<std::string, CellStore>::iterator result = mInteriors.find (lowerName);

    if (result==mInteriors.end())
    {
        const ESM::Cell *cell = mStore.get<ESM::Cell>().find(lowerName);

        result = mInteriors.insert (std::make_pair (lowerName, CellStore (cell))).first;
    }

    if (result->second.mState!=CellStore::State_Loaded)
    {
        result->second.load (mStore, mReader);
    }

    return &result->second;
}

MWWorld::Ptr MWWorld::Cells::getPtr (const std::string& name, CellStore& cell,
    bool searchInContainers)
{
    if (cell.mState==CellStore::State_Unloaded)
        cell.preload (mStore, mReader);

    if (cell.mState==CellStore::State_Preloaded)
    {
        if (std::binary_search (cell.mIds.begin(), cell.mIds.end(), name))
        {
            cell.load (mStore, mReader);
        }
        else
            return Ptr();
    }

    if (MWWorld::LiveCellRef<ESM::Activator> *ref = cell.mActivators.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::Potion> *ref = cell.mPotions.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::Apparatus> *ref = cell.mAppas.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::Armor> *ref = cell.mArmors.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::Book> *ref = cell.mBooks.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::Clothing> *ref = cell.mClothes.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::Container> *ref = cell.mContainers.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::Creature> *ref = cell.mCreatures.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::Door> *ref = cell.mDoors.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::Ingredient> *ref = cell.mIngreds.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::CreatureLevList> *ref = cell.mCreatureLists.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::ItemLevList> *ref = cell.mItemLists.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::Light> *ref = cell.mLights.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::Lockpick> *ref = cell.mLockpicks.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::Miscellaneous> *ref = cell.mMiscItems.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::NPC> *ref = cell.mNpcs.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::Probe> *ref = cell.mProbes.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::Repair> *ref = cell.mRepairs.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::Static> *ref = cell.mStatics.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::Weapon> *ref = cell.mWeapons.find (name))
        return Ptr (ref, &cell);

    if (searchInContainers)
        return cell.searchInContainer (name);

    return Ptr();
}

MWWorld::Ptr MWWorld::Cells::getPtr (const std::string& name)
{
    // First check the cache
    for (std::vector<std::pair<std::string, CellStore *> >::iterator iter (mIdCache.begin());
        iter!=mIdCache.end(); ++iter)
        if (iter->first==name && iter->second)
        {
            Ptr ptr = getPtr (name, *iter->second);
            if (!ptr.isEmpty())
                return ptr;
        }

    // Then check cells that are already listed
    for (std::map<std::pair<int, int>, CellStore>::iterator iter = mExteriors.begin();
        iter!=mExteriors.end(); ++iter)
    {
        Ptr ptr = getPtrAndCache (name, iter->second);
        if (!ptr.isEmpty())
            return ptr;
    }

    for (std::map<std::string, CellStore>::iterator iter = mInteriors.begin();
        iter!=mInteriors.end(); ++iter)
    {
        Ptr ptr = getPtrAndCache (name, iter->second);
        if (!ptr.isEmpty())
            return ptr;
    }

    // Now try the other cells
    const MWWorld::Store<ESM::Cell> &cells = mStore.get<ESM::Cell>();
    MWWorld::Store<ESM::Cell>::iterator iter;

    for (iter = cells.extBegin(); iter != cells.extEnd(); ++iter)
    {
        CellStore *cellStore = getCellStore (&(*iter));

        Ptr ptr = getPtrAndCache (name, *cellStore);

        if (!ptr.isEmpty())
            return ptr;
    }

    for (iter = cells.intBegin(); iter != cells.intEnd(); ++iter)
    {
        CellStore *cellStore = getCellStore (&(*iter));

        Ptr ptr = getPtrAndCache (name, *cellStore);

        if (!ptr.isEmpty())
            return ptr;
    }

    // giving up
    return Ptr();
}

void MWWorld::Cells::getExteriorPtrs(const std::string &name, std::vector<MWWorld::Ptr> &out)
{
    for (std::map<std::pair<int, int>, CellStore>::iterator iter = mExteriors.begin();
        iter!=mExteriors.end(); ++iter)
    {
        Ptr ptr = getPtrAndCache (name, iter->second);
        if (!ptr.isEmpty())
            out.push_back(ptr);
    }

}
