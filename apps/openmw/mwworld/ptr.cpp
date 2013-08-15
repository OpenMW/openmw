
#include "ptr.hpp"

#include <cassert>

#include "containerstore.hpp"

const std::string MWWorld::Ptr::sEmptyString;

ESM::CellRef& MWWorld::Ptr::getCellRef() const
{
    assert(mRef);

    if (mContainerStore)
        mContainerStore->flagAsModified();

    return mRef->mRef;
}

MWWorld::RefData& MWWorld::Ptr::getRefData() const
{
    assert(mRef);

    if (mContainerStore)
        mContainerStore->flagAsModified();

    return mRef->mData;
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
