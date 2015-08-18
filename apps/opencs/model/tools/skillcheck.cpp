#include "skillcheck.hpp"

#include <sstream>

#include <components/esm/loadskil.hpp>

#include "../world/universalid.hpp"

CSMTools::SkillCheckStage::SkillCheckStage (const CSMWorld::IdCollection<ESM::Skill>& skills)
: mSkills (skills)
{}

int CSMTools::SkillCheckStage::setup()
{
    return mSkills.getSize();
}

void CSMTools::SkillCheckStage::perform (int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<ESM::Skill>& record = mSkills.getRecord (stage);

    if (record.isDeleted())
        return;

    const ESM::Skill& skill = record.get();

    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Skill, skill.mId);

    for (int i=0; i<4; ++i)
        if (skill.mData.mUseValue[i]<0)
        {
            std::ostringstream stream;

            stream << "Use value #" << i << " of " << skill.mId << " is negative";

            messages.push_back (std::make_pair (id, stream.str()));
        }

    if (skill.mDescription.empty())
        messages.push_back (std::make_pair (id, skill.mId + " has an empty description"));
}
