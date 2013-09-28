
#include "ptr.hpp"

#include <cassert>

#include "containerstore.hpp"

ESM::CellRef& MWWorld::Ptr::getCellRef() const
{
    assert (mCellRef);

    if (mContainerStore)
        mContainerStore->flagAsModified();

    return *mCellRef;
}

MWWorld::RefData& MWWorld::Ptr::getRefData() const
{
    assert (mRefData);

    if (mContainerStore)
        mContainerStore->flagAsModified();

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
