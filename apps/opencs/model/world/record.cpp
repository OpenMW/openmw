
#include "record.hpp"

CSMWorld::RecordBase::~RecordBase() {}

bool CSMWorld::RecordBase::RecordBase::isDeleted() const
{
    return mState==State_Deleted || mState==State_Erased;
}


bool CSMWorld::RecordBase::RecordBase::isErased() const
{
    return mState==State_Erased;
}


bool CSMWorld::RecordBase::RecordBase::isModified() const
{
    return mState==State_Modified || mState==State_ModifiedOnly;
}