
#include "livecellref.hpp"

#include <components/esm/objectstate.hpp>

#include "ptr.hpp"
#include "class.hpp"

void MWWorld::LiveCellRefBase::loadImp (const ESM::ObjectState& state)
{
    mRef = state.mRef;
    mData = RefData (state);

    Ptr ptr (this);

    if (state.mHasLocals)
        mData.setLocals (state.mLocals, mClass->getScript (ptr));

    mClass->readAdditionalState (ptr, state);
}

void MWWorld::LiveCellRefBase::saveImp (ESM::ObjectState& state) const
{
    state.mRef = mRef;

    /// \todo get rid of this cast once const-correct Ptr are available
    Ptr ptr (const_cast<LiveCellRefBase *> (this));

    mData.write (state, mClass->getScript (ptr));

    mClass->writeAdditionalState (ptr, state);
}

bool MWWorld::LiveCellRefBase::checkStateImp (const ESM::ObjectState& state)
{
    return true;
}