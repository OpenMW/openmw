#include "soundcheck.hpp"

#include <sstream>

#include <components/esm/loadskil.hpp>

#include "../prefs/state.hpp"

#include "../world/universalid.hpp"

CSMTools::SoundCheckStage::SoundCheckStage (const CSMWorld::IdCollection<ESM::Sound>& sounds)
: mSounds (sounds)
{
    mIgnoreBaseRecords = false;
}

int CSMTools::SoundCheckStage::setup()
{
    mIgnoreBaseRecords = CSMPrefs::get()["Reports"]["ignore-base-records"].isTrue();

    return mSounds.getSize();
}

void CSMTools::SoundCheckStage::perform (int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<ESM::Sound>& record = mSounds.getRecord (stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && record.mState == CSMWorld::RecordBase::State_BaseOnly) || record.isDeleted())
        return;

    const ESM::Sound& sound = record.get();

    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Sound, sound.mId);

    if (sound.mData.mMinRange>sound.mData.mMaxRange)
        messages.push_back (std::make_pair (id, "Minimum range larger than maximum range"));

    /// \todo check, if the sound file exists
}
