
#include "ptr.hpp"

#include <cassert>

ESM::CellRef& MWWorld::Ptr::getCellRef() const
{
    assert (mCellRef);
    return *mCellRef;
}

MWWorld::RefData& MWWorld::Ptr::getRefData() const
{
    assert (mRefData);
    return *mRefData;
}

void MWWorld::Ptr::setContainerStore (ContainerStore *store)
{
    assert (store);
    assert (!mCell);

    mContainerStore = store;
}

MWWorld::ContainerStore *MWWorld::Ptr::getContainerStore() const
{
    return mContainerStore;
}
