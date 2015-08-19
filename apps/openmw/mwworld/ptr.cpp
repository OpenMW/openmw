#include "ptr.hpp"

#include <cassert>

#include "containerstore.hpp"
#include "class.hpp"
#include "livecellref.hpp"

const std::string& MWWorld::Ptr::getTypeName() const
{
    if(mRef != 0)
        return mRef->mClass->getTypeName();
    throw std::runtime_error("Can't get type name from an empty object.");
}

MWWorld::LiveCellRefBase *MWWorld::Ptr::getBase() const
{
    if (!mRef)
        throw std::runtime_error ("Can't access cell ref pointed to by null Ptr");

    return mRef;
}

MWWorld::CellRef& MWWorld::Ptr::getCellRef() const
{
    assert(mRef);

    return mRef->mRef;
}

MWWorld::RefData& MWWorld::Ptr::getRefData() const
{
    assert(mRef);

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

MWWorld::Ptr::operator const void *()
{
    return mRef;
}
