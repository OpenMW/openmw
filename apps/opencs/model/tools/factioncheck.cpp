
#include "factioncheck.hpp"

#include <sstream>
#include <map>

#include <components/esm/loadfact.hpp>
#include <components/esm/loadskil.hpp>

#include "../world/universalid.hpp"

CSMTools::FactionCheckStage::FactionCheckStage (const CSMWorld::IdCollection<ESM::Faction>& factions)
: mFactions (factions)
{}

int CSMTools::FactionCheckStage::setup()
{
    return mFactions.getSize();
}

void CSMTools::FactionCheckStage::perform (int stage, std::vector<std::string>& messages)
{
    const CSMWorld::Record<ESM::Faction>& record = mFactions.getRecord (stage);

    if (record.isDeleted())
        return;

    const ESM::Faction& faction = record.get();

    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Faction, faction.mId);

    // test for empty name
    if (faction.mName.empty())
        messages.push_back (id.toString() + "|" + faction.mId + " has an empty name");

    // test for invalid attributes
    if (faction.mData.mAttribute[0]==faction.mData.mAttribute[1] && faction.mData.mAttribute[0]!=-1)
    {
        std::ostringstream stream;

        stream << id.toString() << "|Faction lists same attribute twice";

        messages.push_back (stream.str());
    }

    // test for non-unique skill
    std::map<int, int> skills; // ID, number of occurrences

    for (int i=0; i<6; ++i)
        if (faction.mData.mSkills[i]!=-1)
            ++skills[faction.mData.mSkills[i]];

    for (std::map<int, int>::const_iterator iter (skills.begin()); iter!=skills.end(); ++iter)
        if (iter->second>1)
        {
            std::ostringstream stream;

            stream
                << id.toString() << "|"
                << ESM::Skill::indexToId (iter->first) << " is listed more than once";

            messages.push_back (stream.str());
        }

    /// \todo check data members that can't be edited in the table view
}