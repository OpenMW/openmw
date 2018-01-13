#include "quicksavemanager.hpp"

#include <sstream>

MWState::QuickSaveManager::QuickSaveManager(std::string &saveName, int maxSaves)
{
    this->mSaveName = saveName;
    this->mMaxSaves = maxSaves;
    this->mOldestSlotVisited = NULL;
    this->mOldestSlotId = 0;
    this->mSlotsVisited = 0;
}

void MWState::QuickSaveManager::visitSave(const Slot *saveSlot)
{
    int slotId;
    if(tryExtractSlotId(saveSlot->mProfile.mDescription, slotId))
    {
        ++mSlotsVisited;
        if(isOldestSave(saveSlot))
        {
            mOldestSlotVisited = saveSlot;
            mOldestSlotId = slotId;
        }
    }
}

bool MWState::QuickSaveManager::isOldestSave(const Slot *compare)
{
    if(mOldestSlotVisited == NULL)
        return true;
    return (compare->mTimeStamp <= mOldestSlotVisited->mTimeStamp);
}

bool MWState::QuickSaveManager::tryExtractSlotId(const std::string &slotName, int &extractedId)
{
    std::istringstream formattedExtractor(slotName);

    std::string nameToTest;
    formattedExtractor >> nameToTest;
    if(nameToTest == mSaveName)
    {
        //Only try to extract the id if maxSaves > 1
        //With maxSaves == 1, we don't append the slotId to the name
        if(formattedExtractor >> extractedId)
            return (isSlotIdValid(extractedId));
        else if(mMaxSaves == 1)
            return formattedExtractor.eof();
    }
    return false;
}

bool MWState::QuickSaveManager::isSlotIdValid(int slotId)
{
    return (slotId > 0 && slotId <= mMaxSaves);
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

std::string MWState::QuickSaveManager::getNextQuickSaveName()
{
    std::ostringstream nameFormatter;
    nameFormatter << mSaveName;
    //Only print the number if there will be more than 1
    if(mMaxSaves > 1)
        nameFormatter << " " << calcNextSlotId();
    return nameFormatter.str();
}

int MWState::QuickSaveManager::calcNextSlotId()
{
    if(shouldCreateNewSlot())
        return (mSlotsVisited + 1);
    return mOldestSlotId;
}
