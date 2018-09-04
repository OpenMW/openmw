#include "classcheck.hpp"

#include <map>

#include <components/esm/loadclas.hpp>
#include <components/esm/loadskil.hpp>

#include "../prefs/state.hpp"

#include "../world/universalid.hpp"

CSMTools::ClassCheckStage::ClassCheckStage (const CSMWorld::IdCollection<ESM::Class>& classes)
: mClasses (classes)
{
    mIgnoreBaseRecords = false;
}

int CSMTools::ClassCheckStage::setup()
{
    mIgnoreBaseRecords = CSMPrefs::get()["Reports"]["ignore-base-records"].isTrue();

    return mClasses.getSize();
}

void CSMTools::ClassCheckStage::perform (int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<ESM::Class>& record = mClasses.getRecord (stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && record.mState == CSMWorld::RecordBase::State_BaseOnly) || record.isDeleted())
        return;

    const ESM::Class& class_ = record.get();

    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Class, class_.mId);

    // A class should have a name
    if (class_.mName.empty())
        messages.add(id, "Name is missing", "", CSMDoc::Message::Severity_Error);

    // A playable class should have a description
    if (class_.mData.mIsPlayable != 0 && class_.mDescription.empty())
        messages.add(id, "Description of a playable class is missing", "", CSMDoc::Message::Severity_Warning);

    // test for invalid attributes
    for (int i=0; i<2; ++i)
        if (class_.mData.mAttribute[i]==-1)
        {
            messages.add(id, "Attribute #" + std::to_string(i) + " is not set", "", CSMDoc::Message::Severity_Error);
        }

    if (class_.mData.mAttribute[0]==class_.mData.mAttribute[1] && class_.mData.mAttribute[0]!=-1)
    {
        messages.add(id, "Same attribute is listed twice", "", CSMDoc::Message::Severity_Error);
    }

    // test for non-unique skill
    std::map<int, int> skills; // ID, number of occurrences

    for (int i=0; i<5; ++i)
        for (int i2=0; i2<2; ++i2)
            ++skills[class_.mData.mSkills[i][i2]];

    for (auto &skill : skills)
        if (skill.second>1)
        {
            messages.add(id, "Skill " + ESM::Skill::indexToId (skill.first) + " is listed more than once", "", CSMDoc::Message::Severity_Error);
        }
}
