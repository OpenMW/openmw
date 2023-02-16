#include "journalcheck.hpp"

#include <memory>
#include <set>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <apps/opencs/model/doc/messages.hpp>
#include <apps/opencs/model/prefs/category.hpp>
#include <apps/opencs/model/prefs/setting.hpp>
#include <apps/opencs/model/world/idcollection.hpp>
#include <apps/opencs/model/world/info.hpp>
#include <apps/opencs/model/world/infocollection.hpp>
#include <apps/opencs/model/world/record.hpp>
#include <apps/opencs/model/world/universalid.hpp>

#include <components/esm3/loaddial.hpp>
#include <components/esm3/loadinfo.hpp>

#include "../prefs/state.hpp"

CSMTools::JournalCheckStage::JournalCheckStage(
    const CSMWorld::IdCollection<ESM::Dialogue>& journals, const CSMWorld::InfoCollection& journalInfos)
    : mJournals(journals)
    , mJournalInfos(journalInfos)
{
    mIgnoreBaseRecords = false;
}

int CSMTools::JournalCheckStage::setup()
{
    mIgnoreBaseRecords = CSMPrefs::get()["Reports"]["ignore-base-records"].isTrue();
    mInfosByTopic = mJournalInfos.getInfosByTopic();
    return mJournals.getSize();
}

void CSMTools::JournalCheckStage::perform(int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<ESM::Dialogue>& journalRecord = mJournals.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && journalRecord.mState == CSMWorld::RecordBase::State_BaseOnly)
        || journalRecord.isDeleted())
        return;

    const ESM::Dialogue& journal = journalRecord.get();
    int statusNamedCount = 0;
    int totalInfoCount = 0;
    std::set<int> questIndices;

    if (const auto infos = mInfosByTopic.find(journal.mId); infos != mInfosByTopic.end())
    {
        for (const CSMWorld::Record<CSMWorld::Info>* record : infos->second)
        {
            if (record->isDeleted())
                continue;

            const CSMWorld::Info& journalInfo = record->get();

            totalInfoCount += 1;

            if (journalInfo.mQuestStatus == ESM::DialInfo::QS_Name)
            {
                statusNamedCount += 1;
            }

            // Skip "Base" records (setting!)
            if (mIgnoreBaseRecords && record->mState == CSMWorld::RecordBase::State_BaseOnly)
                continue;

            if (journalInfo.mResponse.empty())
            {
                CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_JournalInfo, journalInfo.mId);
                messages.add(id, "Missing journal entry text", "", CSMDoc::Message::Severity_Warning);
            }

            // Duplicate index
            if (!questIndices.insert(journalInfo.mData.mJournalIndex).second)
            {
                CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_JournalInfo, journalInfo.mId);
                messages.add(id, "Duplicated quest index " + std::to_string(journalInfo.mData.mJournalIndex), "",
                    CSMDoc::Message::Severity_Error);
            }
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
