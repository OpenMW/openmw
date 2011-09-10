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
