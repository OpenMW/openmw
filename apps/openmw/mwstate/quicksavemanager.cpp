#include "quicksavemanager.hpp"

MWState::QuickSaveManager::QuickSaveManager(std::string &saveName, unsigned int maxSaves)
  : mSaveName(saveName)
  , mMaxSaves(maxSaves)
  , mSlotsVisited(0)
  , mOldestSlotVisited(nullptr)
{
}

void MWState::QuickSaveManager::visitSave(const Slot *saveSlot)
{
    if(mSaveName == saveSlot->mProfile.mDescription)
    {
        ++mSlotsVisited;
        if(isOldestSave(saveSlot))
            mOldestSlotVisited = saveSlot;
    }
}

bool MWState::QuickSaveManager::isOldestSave(const Slot *compare)
{
    if(mOldestSlotVisited == NULL)
        return true;
    return (compare->mTimeStamp <= mOldestSlotVisited->mTimeStamp);
}

bool MWState::QuickSaveManager::shouldCreateNewSlot()
{
    return (mSlotsVisited < mMaxSaves);
}

const MWState::Slot *MWState::QuickSaveManager::getNextQuickSaveSlot()
{
    if(shouldCreateNewSlot())
        return NULL;
    return mOldestSlotVisited;
}
