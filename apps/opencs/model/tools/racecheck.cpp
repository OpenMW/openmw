#include "racecheck.hpp"

#include <sstream>

#include <components/esm/loadrace.hpp>

#include "../prefs/state.hpp"

#include "../world/universalid.hpp"

void CSMTools::RaceCheckStage::performPerRecord (int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<ESM::Race>& record = mRaces.getRecord (stage);

    if (record.isDeleted())
        return;

    const ESM::Race& race = record.get();

    // Consider mPlayable flag even when "Base" records are ignored
    if (race.mData.mFlags & 0x1)
        mPlayable = true;

    // Skip "Base" records (setting!)
    if (mIgnoreBaseRecords && record.mState == CSMWorld::RecordBase::State_BaseOnly)
        return;

    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Race, race.mId);

    // test for empty name and description
    if (race.mName.empty())
        messages.push_back (std::make_pair (id, "Name is missing"));

    if (race.mDescription.empty())
        messages.push_back (std::make_pair (id, "Description is missing"));

    // test for positive height
    if (race.mData.mHeight.mMale<=0)
        messages.push_back (std::make_pair (id, "Male height is non-positive"));

    if (race.mData.mHeight.mFemale<=0)
        messages.push_back (std::make_pair (id, "Female height is non-positive"));

    // test for non-negative weight
    if (race.mData.mWeight.mMale<0)
        messages.push_back (std::make_pair (id, "Male weight is negative"));

    if (race.mData.mWeight.mFemale<0)
        messages.push_back (std::make_pair (id, "Female weight is negative"));

    /// \todo check data members that can't be edited in the table view
}

void CSMTools::RaceCheckStage::performFinal (CSMDoc::Messages& messages)
{
    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Races);

    if (!mPlayable)
        messages.push_back (std::make_pair (id, "No playable race"));
}

CSMTools::RaceCheckStage::RaceCheckStage (const CSMWorld::IdCollection<ESM::Race>& races)
: mRaces (races), mPlayable (false)
{
    mIgnoreBaseRecords = false;
}

int CSMTools::RaceCheckStage::setup()
{
    mPlayable = false;
    mIgnoreBaseRecords = CSMPrefs::get()["Reports"]["ignore-base-records"].isTrue();

    return mRaces.getSize()+1;
}

void CSMTools::RaceCheckStage::perform (int stage, CSMDoc::Messages& messages)
{
    if (stage==mRaces.getSize())
        performFinal (messages);
    else
        performPerRecord (stage, messages);
}
