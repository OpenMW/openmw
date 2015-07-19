
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

template<>
void CSMWorld::setRecordDeleted(CSMWorld::Land &land, bool isDeleted)
{
    land.mLand->mIsDeleted = isDeleted;
}

template<>
void CSMWorld::setRecordDeleted(ESM::GameSetting &setting, bool isDeleted)
{
    // GameSetting doesn't have a Deleted flag
}

template<>
void CSMWorld::setRecordDeleted(ESM::MagicEffect &effect, bool isDeleted)
{
    // MagicEffect doesn't have a Deleted flag
}

template<>
void CSMWorld::setRecordDeleted(ESM::Skill &skill, bool isDeleted)
{
    // Skill doesn't have a Deleted flag
}
