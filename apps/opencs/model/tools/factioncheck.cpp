#include "factioncheck.hpp"

#include <map>
#include <string>
#include <utility>

#include <apps/opencs/model/doc/messages.hpp>
#include <apps/opencs/model/prefs/category.hpp>
#include <apps/opencs/model/prefs/setting.hpp>
#include <apps/opencs/model/world/idcollection.hpp>
#include <apps/opencs/model/world/record.hpp>
#include <apps/opencs/model/world/universalid.hpp>

#include <components/esm3/loadfact.hpp>
#include <components/esm3/loadskil.hpp>

#include "../prefs/state.hpp"

CSMTools::FactionCheckStage::FactionCheckStage(const CSMWorld::IdCollection<ESM::Faction>& factions)
    : mFactions(factions)
{
    mIgnoreBaseRecords = false;
}

int CSMTools::FactionCheckStage::setup()
{
    mIgnoreBaseRecords = CSMPrefs::get()["Reports"]["ignore-base-records"].isTrue();

    return mFactions.getSize();
}

void CSMTools::FactionCheckStage::perform(int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<ESM::Faction>& record = mFactions.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && record.mState == CSMWorld::RecordBase::State_BaseOnly) || record.isDeleted())
        return;

    const ESM::Faction& faction = record.get();

    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Faction, faction.mId);

    // test for empty name
    if (faction.mName.empty())
        messages.add(id, "Name is missing", "", CSMDoc::Message::Severity_Error);

    // test for invalid attributes
    std::map<int, int> attributeCount;
    for (size_t i = 0; i < faction.mData.mAttribute.size(); ++i)
    {
        int attribute = faction.mData.mAttribute[i];
        if (attribute != -1)
        {
            auto it = attributeCount.find(attribute);
            if (it == attributeCount.end())
                attributeCount.emplace(attribute, 1);
            else
            {
                if (it->second == 1)
                    messages.add(id, "Same attribute is listed twice", {}, CSMDoc::Message::Severity_Error);
                ++it->second;
            }
        }
    }

    // test for non-unique skill
    std::map<int, int> skills; // ID, number of occurrences

    for (int skill : faction.mData.mSkills)
        if (skill != -1)
            ++skills[skill];

    for (auto& skill : skills)
        if (skill.second > 1)
        {
            messages.add(id, "Skill " + ESM::Skill::indexToRefId(skill.first).toString() + " is listed more than once",
                "", CSMDoc::Message::Severity_Error);
        }

    /// \todo check data members that can't be edited in the table view
}
