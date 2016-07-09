#include "journalcheck.hpp"

#include <set>
#include <sstream>

CSMTools::JournalCheckStage::JournalCheckStage(const CSMWorld::IdCollection<ESM::Dialogue> &journals,
    const CSMWorld::InfoCollection& journalInfos)
    : mJournals(journals), mJournalInfos(journalInfos)
{}

int CSMTools::JournalCheckStage::setup()
{
    return mJournals.getSize();
}

void CSMTools::JournalCheckStage::perform(int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<ESM::Dialogue> &journalRecord = mJournals.getRecord(stage);

    if (journalRecord.isDeleted())
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

        if (journalInfo.mResponse.empty())
        {
            CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_JournalInfo, journalInfo.mId);

            messages.add(id, "Journal Info: missing description", "", CSMDoc::Message::Severity_Warning);
        }

        std::pair<std::set<int>::iterator, bool> result = questIndices.insert(journalInfo.mData.mJournalIndex);

        // Duplicate index
        if (result.second == false)
        {
            CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_JournalInfo, journalInfo.mId);

            std::ostringstream stream;
            stream << "Journal: duplicated quest index " << journalInfo.mData.mJournalIndex;

            messages.add(id, stream.str(), "", CSMDoc::Message::Severity_Error);
        }
    }

    if (totalInfoCount == 0)
    {
        CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Journal, journal.mId);

        messages.add(id, "Journal: no defined Journal Infos", "", CSMDoc::Message::Severity_Warning);
    }
    else if (statusNamedCount > 1)
    {
        CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Journal, journal.mId);

        messages.add(id, "Journal: multiple infos with quest status \"Named\"", "", CSMDoc::Message::Severity_Error);
    }
}
