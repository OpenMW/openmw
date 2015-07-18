
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

template<>
bool CSMWorld::isRecordDeleted(const CSMWorld::Land &land)
{
    return land.mLand->mIsDeleted;
}

template<>
bool CSMWorld::isRecordDeleted(const ESM::GameSetting &setting)
{
    return false;
}

template<>
bool CSMWorld::isRecordDeleted(const ESM::MagicEffect &effect)
{
    return false;
}

template<>
bool CSMWorld::isRecordDeleted(const ESM::Skill &skill)
{
    return false;
}
