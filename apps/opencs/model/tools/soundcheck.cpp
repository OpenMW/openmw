#include "soundcheck.hpp"

#include "../prefs/state.hpp"

#include "../world/universalid.hpp"

CSMTools::SoundCheckStage::SoundCheckStage (const CSMWorld::IdCollection<ESM::Sound> &sounds,
                                            const CSMWorld::Resources &soundfiles)
    : mSounds (sounds),
      mSoundFiles (soundfiles)
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
    {
        messages.add(id, "Minimum range is larger than maximum range", "", CSMDoc::Message::Severity_Warning);
    }

    if (sound.mSound.empty())
    {
        messages.add(id, "Sound file is missing", "", CSMDoc::Message::Severity_Error);
    }
    else if (mSoundFiles.searchId(sound.mSound) == -1)
    {
        messages.add(id, "Sound file '" + sound.mSound + "' does not exist", "", CSMDoc::Message::Severity_Error);
    }
}
