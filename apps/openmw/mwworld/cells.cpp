#include "cells.hpp"

#include <cctype>

#include <algorithm>

#include "world.hpp"
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
    for (ESMS::CellRefList<ESM::Container, RefData>::List::iterator iter (
        cellStore.containers.list.begin());
        iter!=cellStore.containers.list.end(); ++iter)
    {
        Ptr container (&*iter, &cellStore);

        Class::get (container).getContainerStore (container).fill (
            iter->base->inventory, mStore);
    }

    for (ESMS::CellRefList<ESM::Creature, RefData>::List::iterator iter (
        cellStore.creatures.list.begin());
        iter!=cellStore.creatures.list.end(); ++iter)
    {
        Ptr container (&*iter, &cellStore);

        Class::get (container).getContainerStore (container).fill (
            iter->base->inventory, mStore);
    }

    for (ESMS::CellRefList<ESM::NPC, RefData>::List::iterator iter (
        cellStore.npcs.list.begin());
        iter!=cellStore.npcs.list.end(); ++iter)
    {
        Ptr container (&*iter, &cellStore);

        Class::get (container).getContainerStore (container).fill (
            iter->base->inventory, mStore);
    }
}

MWWorld::Cells::Cells (const ESMS::ESMStore& store, ESM::ESMReader& reader, MWWorld::World& world)
: mStore (store), mReader (reader), mWorld (world) {}

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

            cell = mWorld.createRecord (record);
        }

        result = mExteriors.insert (std::make_pair (
            std::make_pair (x, y), Ptr::CellStore (cell))).first;
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
            cell.load (mStore, mReader);
        else
            return Ptr();
    }

    if (ESMS::LiveCellRef<ESM::Activator, RefData> *ref = cell.activators.find (name))
        return Ptr (ref, &cell);

    if (ESMS::LiveCellRef<ESM::Potion, RefData> *ref = cell.potions.find (name))
        return Ptr (ref, &cell);

    if (ESMS::LiveCellRef<ESM::Apparatus, RefData> *ref = cell.appas.find (name))
        return Ptr (ref, &cell);

    if (ESMS::LiveCellRef<ESM::Armor, RefData> *ref = cell.armors.find (name))
        return Ptr (ref, &cell);

    if (ESMS::LiveCellRef<ESM::Book, RefData> *ref = cell.books.find (name))
        return Ptr (ref, &cell);

    if (ESMS::LiveCellRef<ESM::Clothing, RefData> *ref = cell.clothes.find (name))
        return Ptr (ref, &cell);

    if (ESMS::LiveCellRef<ESM::Container, RefData> *ref = cell.containers.find (name))
        return Ptr (ref, &cell);

    if (ESMS::LiveCellRef<ESM::Creature, RefData> *ref = cell.creatures.find (name))
        return Ptr (ref, &cell);

    if (ESMS::LiveCellRef<ESM::Door, RefData> *ref = cell.doors.find (name))
        return Ptr (ref, &cell);

    if (ESMS::LiveCellRef<ESM::Ingredient, RefData> *ref = cell.ingreds.find (name))
        return Ptr (ref, &cell);

    if (ESMS::LiveCellRef<ESM::CreatureLevList, RefData> *ref = cell.creatureLists.find (name))
        return Ptr (ref, &cell);

    if (ESMS::LiveCellRef<ESM::ItemLevList, RefData> *ref = cell.itemLists.find (name))
        return Ptr (ref, &cell);

    if (ESMS::LiveCellRef<ESM::Light, RefData> *ref = cell.lights.find (name))
        return Ptr (ref, &cell);

    if (ESMS::LiveCellRef<ESM::Tool, RefData> *ref = cell.lockpicks.find (name))
        return Ptr (ref, &cell);

    if (ESMS::LiveCellRef<ESM::Miscellaneous, RefData> *ref = cell.miscItems.find (name))
        return Ptr (ref, &cell);

    if (ESMS::LiveCellRef<ESM::NPC, RefData> *ref = cell.npcs.find (name))
        return Ptr (ref, &cell);

    if (ESMS::LiveCellRef<ESM::Probe, RefData> *ref = cell.probes.find (name))
        return Ptr (ref, &cell);

    if (ESMS::LiveCellRef<ESM::Repair, RefData> *ref = cell.repairs.find (name))
        return Ptr (ref, &cell);

    if (ESMS::LiveCellRef<ESM::Static, RefData> *ref = cell.statics.find (name))
        return Ptr (ref, &cell);

    if (ESMS::LiveCellRef<ESM::Weapon, RefData> *ref = cell.weapons.find (name))
        return Ptr (ref, &cell);

    return Ptr();
}

MWWorld::Ptr MWWorld::Cells::getPtr (const std::string& name)
{
    // First check cells that are already listed
    for (std::map<std::string, Ptr::CellStore>::iterator iter = mInteriors.begin();
        iter!=mInteriors.end(); ++iter)
    {
        Ptr ptr = getPtr (name, iter->second);
        if (!ptr.isEmpty())
            return ptr;
    }

    for (std::map<std::pair<int, int>, Ptr::CellStore>::iterator iter = mExteriors.begin();
        iter!=mExteriors.end(); ++iter)
    {
        Ptr ptr = getPtr (name, iter->second);
        if (!ptr.isEmpty())
            return ptr;
    }

    // Now try the other cells
    for (ESMS::CellList::IntCells::const_iterator iter = mStore.cells.intCells.begin();
        iter!=mStore.cells.intCells.end(); ++iter)
    {
        Ptr::CellStore *cellStore = getCellStore (iter->second);

        Ptr ptr = getPtr (name, *cellStore);

        if (!ptr.isEmpty())
            return ptr;
    }

    for (ESMS::CellList::ExtCells::const_iterator iter = mStore.cells.extCells.begin();
        iter!=mStore.cells.extCells.end(); ++iter)
    {
        Ptr::CellStore *cellStore = getCellStore (iter->second);

        Ptr ptr = getPtr (name, *cellStore);

        if (!ptr.isEmpty())
            return ptr;
    }

    // giving up
    return Ptr();
}
