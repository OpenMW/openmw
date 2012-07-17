#include "cells.hpp"

#include <components/esm_store/store.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "class.hpp"
#include "containerstore.hpp"

MWWorld::Ptr::CellStore *MWWorld::Cells::getCellStore (const ESM::Cell *cell)
{
    if (cell->data.flags & ESM::Cell::Interior)
    {
        std::map<std::string, Ptr::CellStore>::iterator result = mInteriors.find (cell->name);

        if (result==mInteriors.end())
        {
            result = mInteriors.insert (std::make_pair (cell->name, Ptr::CellStore (cell))).first;
        }

        return &result->second;
    }
    else
    {
        std::map<std::pair<int, int>, Ptr::CellStore>::iterator result =
            mExteriors.find (std::make_pair (cell->data.gridX, cell->data.gridY));

        if (result==mExteriors.end())
        {
            result = mExteriors.insert (std::make_pair (
                std::make_pair (cell->data.gridX, cell->data.gridY), Ptr::CellStore (cell))).first;

        }

        return &result->second;
    }
}

void MWWorld::Cells::fillContainers (Ptr::CellStore& cellStore)
{
    for (CellRefList<ESM::Container>::List::iterator iter (
        cellStore.containers.list.begin());
        iter!=cellStore.containers.list.end(); ++iter)
    {
        Ptr container (&*iter, &cellStore);

        Class::get (container).getContainerStore (container).fill (
            iter->base->inventory, mStore);
    }

    for (CellRefList<ESM::Creature>::List::iterator iter (
        cellStore.creatures.list.begin());
        iter!=cellStore.creatures.list.end(); ++iter)
    {
        Ptr container (&*iter, &cellStore);

        Class::get (container).getContainerStore (container).fill (
            iter->base->inventory, mStore);
    }

    for (CellRefList<ESM::NPC>::List::iterator iter (
        cellStore.npcs.list.begin());
        iter!=cellStore.npcs.list.end(); ++iter)
    {
        Ptr container (&*iter, &cellStore);

        Class::get (container).getContainerStore (container).fill (
            iter->base->inventory, mStore);
    }
}

MWWorld::Ptr MWWorld::Cells::getPtrAndCache (const std::string& name, Ptr::CellStore& cellStore)
{
    Ptr ptr = getPtr (name, cellStore);

    if (!ptr.isEmpty())
    {
        mIdCache[mIdCacheIndex].first = name;
        mIdCache[mIdCacheIndex].second = &cellStore;
        if (++mIdCacheIndex>=mIdCache.size())
            mIdCacheIndex = 0;
    }

    return ptr;
}

MWWorld::Cells::Cells (const ESMS::ESMStore& store, ESM::ESMReader& reader)
: mStore (store), mReader (reader),
  mIdCache (20, std::pair<std::string, Ptr::CellStore *> ("", (Ptr::CellStore*)0)), /// \todo make cache size configurable
  mIdCacheIndex (0)
{}

MWWorld::Ptr::CellStore *MWWorld::Cells::getExterior (int x, int y)
{
    std::map<std::pair<int, int>, Ptr::CellStore>::iterator result =
        mExteriors.find (std::make_pair (x, y));

    if (result==mExteriors.end())
    {
        const ESM::Cell *cell = mStore.cells.searchExt (x, y);

        if (!cell)
        {
            // Cell isn't predefined. Make one on the fly.
            ESM::Cell record;

            record.data.flags = 0;
            record.data.gridX = x;
            record.data.gridY = y;
            record.water = 0;
            record.mapColor = 0;

            cell = MWBase::Environment::get().getWorld()->createRecord (record);
        }

        result = mExteriors.insert (std::make_pair (
            std::make_pair (x, y), CellStore (cell))).first;
    }

    if (result->second.mState!=Ptr::CellStore::State_Loaded)
    {
        result->second.load (mStore, mReader);
        fillContainers (result->second);
    }

    return &result->second;
}

MWWorld::Ptr::CellStore *MWWorld::Cells::getInterior (const std::string& name)
{
    std::map<std::string, Ptr::CellStore>::iterator result = mInteriors.find (name);

    if (result==mInteriors.end())
    {
        const ESM::Cell *cell = mStore.cells.findInt (name);

        result = mInteriors.insert (std::make_pair (name, Ptr::CellStore (cell))).first;
    }

    if (result->second.mState!=Ptr::CellStore::State_Loaded)
    {
        result->second.load (mStore, mReader);
        fillContainers (result->second);
    }

    return &result->second;
}

MWWorld::Ptr MWWorld::Cells::getPtr (const std::string& name, Ptr::CellStore& cell)
{
    if (cell.mState==Ptr::CellStore::State_Unloaded)
        cell.preload (mStore, mReader);

    if (cell.mState==Ptr::CellStore::State_Preloaded)
    {
        std::string lowerCase;

        std::transform (name.begin(), name.end(), std::back_inserter (lowerCase),
            (int(*)(int)) std::tolower);

        if (std::binary_search (cell.mIds.begin(), cell.mIds.end(), lowerCase))
        {
            cell.load (mStore, mReader);
            fillContainers (cell);
        }
        else
            return Ptr();
    }

    if (MWWorld::LiveCellRef<ESM::Activator> *ref = cell.activators.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::Potion> *ref = cell.potions.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::Apparatus> *ref = cell.appas.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::Armor> *ref = cell.armors.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::Book> *ref = cell.books.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::Clothing> *ref = cell.clothes.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::Container> *ref = cell.containers.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::Creature> *ref = cell.creatures.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::Door> *ref = cell.doors.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::Ingredient> *ref = cell.ingreds.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::CreatureLevList> *ref = cell.creatureLists.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::ItemLevList> *ref = cell.itemLists.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::Light> *ref = cell.lights.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::Tool> *ref = cell.lockpicks.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::Miscellaneous> *ref = cell.miscItems.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::NPC> *ref = cell.npcs.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::Probe> *ref = cell.probes.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::Repair> *ref = cell.repairs.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::Static> *ref = cell.statics.find (name))
        return Ptr (ref, &cell);

    if (MWWorld::LiveCellRef<ESM::Weapon> *ref = cell.weapons.find (name))
        return Ptr (ref, &cell);

    return Ptr();
}

MWWorld::Ptr MWWorld::Cells::getPtr (const std::string& name)
{
    // First check the cache
    for (std::vector<std::pair<std::string, Ptr::CellStore *> >::iterator iter (mIdCache.begin());
        iter!=mIdCache.end(); ++iter)
        if (iter->first==name && iter->second)
        {
            Ptr ptr = getPtr (name, *iter->second);
            if (!ptr.isEmpty())
                return ptr;
        }

    // Then check cells that are already listed
    for (std::map<std::string, Ptr::CellStore>::iterator iter = mInteriors.begin();
        iter!=mInteriors.end(); ++iter)
    {
        Ptr ptr = getPtrAndCache (name, iter->second);
        if (!ptr.isEmpty())
             return ptr;
    }

    for (std::map<std::pair<int, int>, Ptr::CellStore>::iterator iter = mExteriors.begin();
        iter!=mExteriors.end(); ++iter)
    {
        Ptr ptr = getPtrAndCache (name, iter->second);
        if (!ptr.isEmpty())
            return ptr;
    }

    // Now try the other cells
    for (ESMS::CellList::IntCells::const_iterator iter = mStore.cells.intCells.begin();
        iter!=mStore.cells.intCells.end(); ++iter)
    {
        Ptr::CellStore *cellStore = getCellStore (iter->second);

        Ptr ptr = getPtrAndCache (name, *cellStore);

        if (!ptr.isEmpty())
            return ptr;
    }

    for (ESMS::CellList::ExtCells::const_iterator iter = mStore.cells.extCells.begin();
        iter!=mStore.cells.extCells.end(); ++iter)
    {
        Ptr::CellStore *cellStore = getCellStore (iter->second);

        Ptr ptr = getPtrAndCache (name, *cellStore);

        if (!ptr.isEmpty())
            return ptr;
    }

    // giving up
    return Ptr();
}
