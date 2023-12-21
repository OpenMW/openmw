#include "racecheck.hpp"

#include <string>

#include "../prefs/state.hpp"

#include "../world/universalid.hpp"

#include <apps/opencs/model/doc/messages.hpp>
#include <apps/opencs/model/prefs/category.hpp>
#include <apps/opencs/model/prefs/setting.hpp>
#include <apps/opencs/model/world/idcollection.hpp>
#include <apps/opencs/model/world/record.hpp>
#include <components/esm3/loadrace.hpp>

void CSMTools::RaceCheckStage::performPerRecord(int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<ESM::Race>& record = mRaces.getRecord(stage);

    if (record.isDeleted())
        return;

    const ESM::Race& race = record.get();

    // Consider mPlayable flag even when "Base" records are ignored
    if (race.mData.mFlags & 0x1)
        mPlayable = true;

    // Skip "Base" records (setting!)
    if (mIgnoreBaseRecords && record.mState == CSMWorld::RecordBase::State_BaseOnly)
        return;

    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Race, race.mId);

    // test for empty name and description
    if (race.mName.empty())
        messages.add(id, "Name is missing", "",
            (race.mData.mFlags & 0x1) ? CSMDoc::Message::Severity_Error : CSMDoc::Message::Severity_Warning);

    if (race.mDescription.empty())
        messages.add(id, "Description is missing", "", CSMDoc::Message::Severity_Warning);

    // test for positive height
    if (race.mData.mMaleHeight <= 0)
        messages.add(id, "Male height is non-positive", "", CSMDoc::Message::Severity_Error);

    if (race.mData.mFemaleHeight <= 0)
        messages.add(id, "Female height is non-positive", "", CSMDoc::Message::Severity_Error);

    // test for non-negative weight
    if (race.mData.mMaleWeight < 0)
        messages.add(id, "Male weight is negative", "", CSMDoc::Message::Severity_Error);

    if (race.mData.mFemaleWeight < 0)
        messages.add(id, "Female weight is negative", "", CSMDoc::Message::Severity_Error);

    /// \todo check data members that can't be edited in the table view
}

void CSMTools::RaceCheckStage::performFinal(CSMDoc::Messages& messages)
{
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Races);

    if (!mPlayable)
        messages.add(id, "No playable race", "", CSMDoc::Message::Severity_SeriousError);
}

CSMTools::RaceCheckStage::RaceCheckStage(const CSMWorld::IdCollection<ESM::Race>& races)
    : mRaces(races)
    , mPlayable(false)
{
    mIgnoreBaseRecords = false;
}

int CSMTools::RaceCheckStage::setup()
{
    mPlayable = false;
    mIgnoreBaseRecords = CSMPrefs::get()["Reports"]["ignore-base-records"].isTrue();

    return mRaces.getSize() + 1;
}

void CSMTools::RaceCheckStage::perform(int stage, CSMDoc::Messages& messages)
{
    if (stage == mRaces.getSize())
        performFinal(messages);
    else
        performPerRecord(stage, messages);
}
