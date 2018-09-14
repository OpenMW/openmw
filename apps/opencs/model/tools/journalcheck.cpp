#include "journalcheck.hpp"

#include <set>

#include "../prefs/state.hpp"

CSMTools::JournalCheckStage::JournalCheckStage(const CSMWorld::IdCollection<ESM::Dialogue> &journals,
    const CSMWorld::InfoCollection& journalInfos)
    : mJournals(journals), mJournalInfos(journalInfos)
{
    mIgnoreBaseRecords = false;
}

int CSMTools::JournalCheckStage::setup()
{
    mIgnoreBaseRecords = CSMPrefs::get()["Reports"]["ignore-base-records"].isTrue();

    return mJournals.getSize();
}

void CSMTools::JournalCheckStage::perform(int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<ESM::Dialogue> &journalRecord = mJournals.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && journalRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || journalRecord.isDeleted())
        return;

    const ESM::Dialogue &journal = journalRecord.get();
    int statusNamedCount = 0;
    int totalInfoCount = 0;
    std::set<int> questIndices;

    CSMWorld::InfoCollection::Range range = mJournalInfos.getTopicRange(journal.mId);

    for (CSMWorld::InfoCollection::RecordConstIterator it = range.first; it != range.second; ++it)
    {
        const CSMWorld::Record<CSMWorld::Info> infoRecord = (*it);

        if (infoRecord.isDeleted())
            continue;

        const CSMWorld::Info& journalInfo = infoRecord.get();

        totalInfoCount += 1;

        if (journalInfo.mQuestStatus == ESM::DialInfo::QS_Name)
        {
            statusNamedCount += 1;
        }

        // Skip "Base" records (setting!)
        if (mIgnoreBaseRecords && infoRecord.mState == CSMWorld::RecordBase::State_BaseOnly)
            continue;

        if (journalInfo.mResponse.empty())
        {
            CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_JournalInfo, journalInfo.mId);
            messages.add(id, "Missing journal entry text", "", CSMDoc::Message::Severity_Warning);
        }

        std::pair<std::set<int>::iterator, bool> result = questIndices.insert(journalInfo.mData.mJournalIndex);

        // Duplicate index
        if (!result.second)
        {
            CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_JournalInfo, journalInfo.mId);
            messages.add(id, "Duplicated quest index " + std::to_string(journalInfo.mData.mJournalIndex), "", CSMDoc::Message::Severity_Error);
        }
    }

    if (totalInfoCount == 0)
    {
        CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Journal, journal.mId);
        messages.add(id, "No related journal entry", "", CSMDoc::Message::Severity_Warning);
    }
    else if (statusNamedCount > 1)
    {
        CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Journal, journal.mId);
        messages.add(id, "Multiple entries with quest status 'Named'", "", CSMDoc::Message::Severity_Error);
    }
}
