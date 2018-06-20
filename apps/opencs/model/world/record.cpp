#include "record.hpp"

CSMWorld::RecordBase::~RecordBase() {}

bool CSMWorld::RecordBase::isDeleted() const
{
    return mState==State_Deleted || mState==State_Erased;
}


bool CSMWorld::RecordBase::isErased() const
{
    return mState==State_Erased;
}


bool CSMWorld::RecordBase::isModified() const
{
    return mState==State_Modified || mState==State_ModifiedOnly;
}