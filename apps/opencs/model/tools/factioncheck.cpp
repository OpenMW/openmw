#include "factioncheck.hpp"

#include <sstream>
#include <map>

#include <components/esm/loadfact.hpp>
#include <components/esm/loadskil.hpp>

#include "../prefs/state.hpp"

#include "../world/universalid.hpp"

CSMTools::FactionCheckStage::FactionCheckStage (const CSMWorld::IdCollection<ESM::Faction>& factions)
: mFactions (factions)
{
    mIgnoreBaseRecords = false;
}

int CSMTools::FactionCheckStage::setup()
{
    mIgnoreBaseRecords = CSMPrefs::get()["Reports"]["ignore-base-records"].isTrue();

    return mFactions.getSize();
}

void CSMTools::FactionCheckStage::perform (int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<ESM::Faction>& record = mFactions.getRecord (stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && record.mState == CSMWorld::RecordBase::State_BaseOnly) || record.isDeleted())
        return;

    const ESM::Faction& faction = record.get();

    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Faction, faction.mId);

    // test for empty name
    if (faction.mName.empty())
        messages.push_back (std::make_pair (id, "Name is missing"));

    // test for invalid attributes
    if (faction.mData.mAttribute[0]==faction.mData.mAttribute[1] && faction.mData.mAttribute[0]!=-1)
    {
        messages.push_back (std::make_pair (id, "Same attribute is listed twice"));
    }

    // test for non-unique skill
    std::map<int, int> skills; // ID, number of occurrences

    for (int i=0; i<7; ++i)
        if (faction.mData.mSkills[i]!=-1)
            ++skills[faction.mData.mSkills[i]];

    for (auto &skill : skills)
        if (skill.second>1)
        {
            messages.push_back (std::make_pair (id, "Skill " + ESM::Skill::indexToId (skill.first) + " is listed more than once"));
        }

    /// \todo check data members that can't be edited in the table view
}
