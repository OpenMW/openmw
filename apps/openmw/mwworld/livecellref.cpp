
#include "livecellref.hpp"

#include <components/esm/objectstate.hpp>

void MWWorld::LiveCellRefBase::loadImp (const ESM::ObjectState& state)
{
    mRef = state.mRef;
    mData = RefData (state);
}

void MWWorld::LiveCellRefBase::saveImp (ESM::ObjectState& state) const
{
    state.mRef = mRef;
    mData.write (state);
}

bool MWWorld::LiveCellRefBase::checkStateImp (const ESM::ObjectState& state)
{
    return true;
}