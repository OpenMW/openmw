
#include "classcheck.hpp"

#include <sstream>
#include <map>

#include <components/esm/loadclas.hpp>
#include <components/esm/loadskil.hpp>

#include "../world/universalid.hpp"

CSMTools::ClassCheckStage::ClassCheckStage (const CSMWorld::IdCollection<ESM::Class>& classes)
: mClasses (classes)
{}

int CSMTools::ClassCheckStage::setup()
{
    return mClasses.getSize();
}

void CSMTools::ClassCheckStage::perform (int stage, std::vector<std::string>& messages)
{
    const CSMWorld::Record<ESM::Class>& record = mClasses.getRecord (stage);

    if (record.isDeleted())
        return;

    const ESM::Class& class_ = record.get();

    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Class, class_.mId);

    // test for empty name and description
    if (class_.mName.empty())
        messages.push_back (id.toString() + "|" + class_.mId + " has an empty name");

    if (class_.mDescription.empty())
        messages.push_back (id.toString() + "|" + class_.mId + " has an empty description");

    // test for invalid attributes
    for (int i=0; i<2; ++i)
        if (class_.mData.mAttribute[i]==-1)
        {
            std::ostringstream stream;

            stream << id.toString() << "|Attribute #" << i << " of " << class_.mId << " is not set";

            messages.push_back (stream.str());
        }

    if (class_.mData.mAttribute[0]==class_.mData.mAttribute[1] && class_.mData.mAttribute[0]!=-1)
    {
        std::ostringstream stream;

        stream << id.toString() << "|Class lists same attribute twice";

        messages.push_back (stream.str());
    }

    // test for non-unique skill
    std::map<int, int> skills; // ID, number of occurrences

    for (int i=0; i<5; ++i)
        for (int i2=0; i2<2; ++i2)
            ++skills[class_.mData.mSkills[i][i2]];

    for (std::map<int, int>::const_iterator iter (skills.begin()); iter!=skills.end(); ++iter)
        if (iter->second>1)
        {
            std::ostringstream stream;

            stream
                << id.toString() << "|"
                << ESM::Skill::indexToId (iter->first) << " is listed more than once";

            messages.push_back (stream.str());
        }
}