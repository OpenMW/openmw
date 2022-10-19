#include "skillcheck.hpp"

#include <string>

#include "../prefs/state.hpp"

#include "../world/universalid.hpp"

#include <apps/opencs/model/doc/messages.hpp>
#include <apps/opencs/model/prefs/category.hpp>
#include <apps/opencs/model/prefs/setting.hpp>
#include <apps/opencs/model/world/idcollection.hpp>
#include <apps/opencs/model/world/record.hpp>
#include <components/esm3/loadskil.hpp>

CSMTools::SkillCheckStage::SkillCheckStage(const CSMWorld::IdCollection<ESM::Skill>& skills)
    : mSkills(skills)
{
    mIgnoreBaseRecords = false;
}

int CSMTools::SkillCheckStage::setup()
{
    mIgnoreBaseRecords = CSMPrefs::get()["Reports"]["ignore-base-records"].isTrue();

    return mSkills.getSize();
}

void CSMTools::SkillCheckStage::perform(int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<ESM::Skill>& record = mSkills.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && record.mState == CSMWorld::RecordBase::State_BaseOnly) || record.isDeleted())
        return;

    const ESM::Skill& skill = record.get();

    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Skill, skill.mId);

    if (skill.mDescription.empty())
        messages.add(id, "Description is missing", "", CSMDoc::Message::Severity_Warning);

    for (int i = 0; i < 4; ++i)
        if (skill.mData.mUseValue[i] < 0)
        {
            messages.add(id, "Use value #" + std::to_string(i) + " is negative", "", CSMDoc::Message::Severity_Error);
        }
}
