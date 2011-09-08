#include "cells.hpp"

MWWorld::Cells::Cells (const ESMS::ESMStore& store, ESM::ESMReader& reader)
: mStore (store), mReader (reader) {}

MWWorld::Ptr::CellStore *MWWorld::Cells::getExterior (int x, int y)
{
    std::map<std::pair<int, int>, Ptr::CellStore>::iterator result =
        mExteriors.find (std::make_pair (x, y));

    if (result==mExteriors.end())
    {
        result = mExteriors.insert (std::make_pair (std::make_pair (x, y), Ptr::CellStore())).first;

        result->second.loadExt (x, y, mStore, mReader);
    }

    return &result->second;
}

MWWorld::Ptr::CellStore *MWWorld::Cells::getInterior (const std::string& name)
{
    std::map<std::string, Ptr::CellStore>::iterator result = mInteriors.find (name);

    if (result==mInteriors.end())
    {
        result = mInteriors.insert (std::make_pair (name, Ptr::CellStore())).first;

        result->second.loadInt (name, mStore, mReader);
    }

    return &result->second;
}
