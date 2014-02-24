
#include "ptr.hpp"

#include <cassert>

#include "containerstore.hpp"
#include "class.hpp"


/* This shouldn't really be here. */
MWWorld::LiveCellRefBase::LiveCellRefBase(std::string type, const ESM::CellRef &cref)
  : mClass(&Class::get(type)), mRef(cref), mData(mRef)
{
}


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

ESM::CellRef& MWWorld::Ptr::getCellRef() const
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