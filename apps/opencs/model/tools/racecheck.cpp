
#include "racecheck.hpp"

#include <sstream>

#include <components/esm/loadrace.hpp>

#include "../world/universalid.hpp"

CSMTools::RaceCheckStage::RaceCheckStage (const CSMWorld::IdCollection<ESM::Race>& races)
: mRaces (races)
{}

int CSMTools::RaceCheckStage::setup()
{
    return mRaces.getSize();
}

void CSMTools::RaceCheckStage::perform (int stage, std::vector<std::string>& messages)
{
    const ESM::Race& race = mRaces.getRecord (stage).get();

    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Race, race.mId);

    // test for empty name and description
    if (race.mName.empty())
        messages.push_back (id.toString() + "|" + race.mId + " has an empty name");

    if (race.mDescription.empty())
        messages.push_back (id.toString() + "|" + race.mId + " has an empty description");

    // test for positive height
    if (race.mData.mHeight.mMale<=0)
        messages.push_back (id.toString() + "|male " + race.mId + " has non-positive height");

    if (race.mData.mHeight.mFemale<=0)
        messages.push_back (id.toString() + "|female " + race.mId + " has non-positive height");

    // test for non-negative weight
    if (race.mData.mWeight.mMale<0)
        messages.push_back (id.toString() + "|male " + race.mId + " has negative weight");

    if (race.mData.mWeight.mFemale<0)
        messages.push_back (id.toString() + "|female " + race.mId + " has negative weight");

    /// \todo check data members that can't be edited in the table view
}