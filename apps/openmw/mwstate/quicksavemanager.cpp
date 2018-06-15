#include "quicksavemanager.hpp"

MWState::NextSlotFinder::NextSlotFinder(const std::string &saveName, unsigned int maxSaves)
  : mSaveName(saveName)
  , mMaxSaves(maxSaves)
  , mSlotsVisited(0)
  , mOldestSlotVisited(nullptr)
{
}

void MWState::NextSlotFinder::visitSave(const Slot *saveSlot)
{
    if(mSaveName == saveSlot->mProfile.mDescription)
    {
        ++mSlotsVisited;
        if(isOldestSave(saveSlot))
            mOldestSlotVisited = saveSlot;
    }
}

bool MWState::NextSlotFinder::isOldestSave(const Slot *compare) const
{
    if(mOldestSlotVisited == NULL)
        return true;
    return (compare->mTimeStamp <= mOldestSlotVisited->mTimeStamp);
}

bool MWState::NextSlotFinder::shouldCreateNewSlot() const
{
    return (mSlotsVisited < mMaxSaves);
}

const MWState::Slot *MWState::NextSlotFinder::getNextQuickSaveSlot() const
{
    if(shouldCreateNewSlot())
        return NULL;
    return mOldestSlotVisited;
}


MWState::LatestSlotFinder::LatestSlotFinder(const std::string &saveName)
    : mSaveName(saveName)
    , mLatestSlotVisited(nullptr)
{
}

void MWState::LatestSlotFinder::visitSave(const Slot *saveSlot)
{
    if (mSaveName == saveSlot->mProfile.mDescription)
    {
        if (isLatestSave(saveSlot))
            mLatestSlotVisited = saveSlot;
    }
}

bool MWState::LatestSlotFinder::isLatestSave(const Slot *compare) const
{
    if (mLatestSlotVisited == nullptr)
        return true;
    return (compare->mTimeStamp > mLatestSlotVisited->mTimeStamp);
}

const MWState::Slot *MWState::LatestSlotFinder::getLatestSaveSlot() const
{
    return mLatestSlotVisited;
}
