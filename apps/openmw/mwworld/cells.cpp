#include "cells.hpp"

MWWorld::Cells::Cells (const ESMS::ESMStore& store, ESM::ESMReader& reader)
: mStore (store), mReader (reader) {}

MWWorld::Ptr::CellStore *MWWorld::Cells::getExterior (int x, int y)
{
    std::map<std::pair<int, int>, Ptr::CellStore>::iterator result =
        mExteriors.find (std::make_pair (x, y));

    if (result==mExteriors.end())
    {
        const ESM::Cell *cell = mStore.cells.findExt (x, y);

        result = mExteriors.insert (std::make_pair (
            std::make_pair (x, y), Ptr::CellStore (cell))).first;

        result->second.load (mStore, mReader);
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

        result->second.load (mStore, mReader);
    }

    return &result->second;
}

MWWorld::Ptr MWWorld::Cells::getPtr (const std::string& name, Ptr::CellStore& cell)
{
    cell.load (mStore, mReader);

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
