#include "classcheck.hpp"

#include <map>
#include <string>
#include <utility>

#include <apps/opencs/model/doc/messages.hpp>
#include <apps/opencs/model/prefs/category.hpp>
#include <apps/opencs/model/prefs/setting.hpp>
#include <apps/opencs/model/world/idcollection.hpp>
#include <apps/opencs/model/world/record.hpp>
#include <apps/opencs/model/world/universalid.hpp>

#include <components/esm3/loadclas.hpp>
#include <components/esm3/loadskil.hpp>

#include "../prefs/state.hpp"

CSMTools::ClassCheckStage::ClassCheckStage(const CSMWorld::IdCollection<ESM::Class>& classes)
    : mClasses(classes)
{
    mIgnoreBaseRecords = false;
}

int CSMTools::ClassCheckStage::setup()
{
    mIgnoreBaseRecords = CSMPrefs::get()["Reports"]["ignore-base-records"].isTrue();

    return mClasses.getSize();
}

void CSMTools::ClassCheckStage::perform(int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<ESM::Class>& record = mClasses.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && record.mState == CSMWorld::RecordBase::State_BaseOnly) || record.isDeleted())
        return;

    const ESM::Class& classRecord = record.get();

    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Class, classRecord.mId);

    // A class should have a name
    if (classRecord.mName.empty())
        messages.add(id, "Name is missing", "", CSMDoc::Message::Severity_Error);

    // A playable class should have a description
    if (classRecord.mData.mIsPlayable != 0 && classRecord.mDescription.empty())
        messages.add(id, "Description of a playable class is missing", "", CSMDoc::Message::Severity_Warning);

    // test for invalid attributes
    std::map<int, int> attributeCount;
    for (size_t i = 0; i < classRecord.mData.mAttribute.size(); ++i)
    {
        int attribute = classRecord.mData.mAttribute[i];
        if (attribute == -1)
            messages.add(id, "Attribute #" + std::to_string(i) + " is not set", {}, CSMDoc::Message::Severity_Error);
        else
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

    for (const auto& s : classRecord.mData.mSkills)
        for (int skill : s)
            ++skills[skill];

    for (auto& skill : skills)
        if (skill.second > 1)
        {
            messages.add(id, "Skill " + ESM::Skill::indexToRefId(skill.first).toString() + " is listed more than once",
                "", CSMDoc::Message::Severity_Error);
        }
}
