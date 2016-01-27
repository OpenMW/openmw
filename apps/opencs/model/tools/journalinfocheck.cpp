#include "journalinfocheck.hpp"

#include <set>

CSMTools::JournalInfoCheckStage::JournalInfoCheckStage(
    const CSMWorld::IdCollection<ESM::Dialogue> &journals,
    const CSMWorld::InfoCollection& journalInfos)
    : mJournals(journals), mJournalInfos(journalInfos)
{}

int CSMTools::JournalInfoCheckStage::setup()
{
    return mJournals.getSize();
}

void CSMTools::JournalInfoCheckStage::perform(int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<ESM::Dialogue> &journalRecord = mJournals.getRecord(stage);
    
    if (journalRecord.isDeleted())
        return;
    
    const ESM::Dialogue &journal = journalRecord.get();
    
    CSMWorld::UniversalId journalId (CSMWorld::UniversalId::Type_Journal, journal.mId);
    
    bool infoWasModified = journalRecord.isModified();
    bool indicesAreRepeated = false;
    int infoCount = 0;
    int infoNameCount = 0;
    int infoFinishedCount = 0;
    std::set<int> infoIndices;
    
    CSMWorld::InfoCollection::Range range = mJournalInfos.getTopicRange(journal.mId);
    
    for (CSMWorld::InfoCollection::RecordConstIterator it = range.first;
         it != range.second; ++it)
    {
        const CSMWorld::Record<CSMWorld::Info> infoRecord = (*it);
        
        if (infoRecord.isDeleted())
            continue;
        else if (infoRecord.isModified())
            infoWasModified = true;
        
        const CSMWorld::Info& info = infoRecord.get();
        
        // Update counters
        
        ++infoCount;
        
        if (info.mQuestStatus == ESM::DialInfo::QS_Name)
            ++infoNameCount;
        else if (info.mQuestStatus == ESM::DialInfo::QS_Finished)
            ++infoFinishedCount;
        
        // Check for duplicate quest ids
            
        std::pair<std::set<int>::iterator, bool> result = 
            infoIndices.insert(info.mData.mJournalIndex);
        
        if (result.second == false)
            indicesAreRepeated = true;
    }
    
    if (infoWasModified) // Ignore all the base game errors, there's a lot
    {
        if (infoCount == 0)
            messages.add(journalId, journal.mId + " has no infos", 
                         "", CSMDoc::Message::Severity_Error);
        
        if (infoNameCount > 1)
            messages.add(journalId, journal.mId + " has multiple infos with quest status \"Named\"", 
                         "", CSMDoc::Message::Severity_Error);
        else if (infoNameCount == 0)
            messages.add(journalId, journal.mId + " has no infos with quest status \"Named\"", 
                         "", CSMDoc::Message::Severity_Error);

        if (infoFinishedCount == 0)
            messages.add(journalId, journal.mId + " has no infos with quest status \"Finished\"", 
                         "", CSMDoc::Message::Severity_Error);
            
        if (indicesAreRepeated)
            messages.add(journalId, journal.mId + " has quest indices that are repeated",
                         "", CSMDoc::Message::Severity_Error);
    }
    
}