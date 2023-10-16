#include "regioncheck.hpp"

#include <string>
#include <vector>

#include "../prefs/state.hpp"

#include "../world/universalid.hpp"

#include <apps/opencs/model/doc/messages.hpp>
#include <apps/opencs/model/prefs/category.hpp>
#include <apps/opencs/model/prefs/setting.hpp>
#include <apps/opencs/model/world/idcollection.hpp>
#include <apps/opencs/model/world/record.hpp>

#include <components/esm3/loadregn.hpp>

CSMTools::RegionCheckStage::RegionCheckStage(const CSMWorld::IdCollection<ESM::Region>& regions)
    : mRegions(regions)
{
    mIgnoreBaseRecords = false;
}

int CSMTools::RegionCheckStage::setup()
{
    mIgnoreBaseRecords = CSMPrefs::get()["Reports"]["ignore-base-records"].isTrue();

    return mRegions.getSize();
}

void CSMTools::RegionCheckStage::perform(int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<ESM::Region>& record = mRegions.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && record.mState == CSMWorld::RecordBase::State_BaseOnly) || record.isDeleted())
        return;

    const ESM::Region& region = record.get();

    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Region, region.mId);

    // test for empty name
    if (region.mName.empty())
        messages.add(id, "Name is missing", "", CSMDoc::Message::Severity_Error);

    /// \todo test that the ID in mSleeplist exists

    // test that chances add up to 100
    auto chances = std::accumulate(region.mData.mProbabilities.begin(), region.mData.mProbabilities.end(), 0u);
    if (chances != 100)
        messages.add(id, "Weather chances do not add up to 100", "", CSMDoc::Message::Severity_Error);

    for (const ESM::Region::SoundRef& sound : region.mSoundList)
    {
        if (sound.mChance > 100)
            messages.add(id, "Chance of '" + sound.mSound.getRefIdString() + "' sound to play is over 100 percent", "",
                CSMDoc::Message::Severity_Warning);
    }

    /// \todo check data members that can't be edited in the table view
}
