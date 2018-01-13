#include "quicksavemanager.hpp"

#include <sstream>

MWState::QuickSaveManager::QuickSaveManager(std::string &saveName, int maxSaves){
    this->saveName = saveName;
    this->maxSaves = maxSaves;
    this->oldestSlotVisited = NULL;
    this->oldestSlotId = 0;
    this->slotsVisited = 0;
}

void MWState::QuickSaveManager::visitSave(const Slot *saveSlot){
    int slotId;
    if(tryExtractSlotId(saveSlot->mProfile.mDescription, slotId)){
        ++slotsVisited;
        if(isOldestSave(saveSlot)){
            oldestSlotVisited = saveSlot;
            oldestSlotId = slotId;
        }
    }
}

bool MWState::QuickSaveManager::isOldestSave(const Slot *compare){
    if(oldestSlotVisited == NULL)
        return true;
    return (compare->mTimeStamp < oldestSlotVisited->mTimeStamp);
}

bool MWState::QuickSaveManager::tryExtractSlotId(const std::string &slotName, int &extractedId){
    std::istringstream formattedExtractor = std::istringstream(slotName);

    std::string nameToTest;
    formattedExtractor >> nameToTest;
    if(nameToTest == saveName){
        //Only try to extract the id if maxSaves > 1
        //With maxSaves == 1, we don't append the slotId to the name
        if(formattedExtractor >> extractedId)
            return (isSlotIdValid(extractedId));
        else if(maxSaves == 1)
            return formattedExtractor.eof();
    }
    return false;
}

bool MWState::QuickSaveManager::isSlotIdValid(int slotId){
    return (slotId > 0 && slotId <= maxSaves);
}

bool MWState::QuickSaveManager::shouldCreateNewSlot(){
    return (slotsVisited < maxSaves);
}

const MWState::Slot *MWState::QuickSaveManager::getNextQuickSaveSlot(){
    if(shouldCreateNewSlot())
        return NULL;
    return oldestSlotVisited;
}

std::string MWState::QuickSaveManager::getNextQuickSaveName(){
    std::ostringstream nameFormatter;
    nameFormatter << saveName;
    //Only print the number if there will be more than 1
    if(maxSaves > 1)
        nameFormatter << " " << calcNextSlotId();
    return nameFormatter.str();
}

int MWState::QuickSaveManager::calcNextSlotId(){
    if(shouldCreateNewSlot())
        return (slotsVisited + 1);
    return oldestSlotId;
}
