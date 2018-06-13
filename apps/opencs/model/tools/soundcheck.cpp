#include "soundcheck.hpp"

#include <sstream>

#include <components/esm/loadskil.hpp>

#include "../world/universalid.hpp"

CSMTools::SoundCheckStage::SoundCheckStage (const CSMWorld::IdCollection<ESM::Sound>& sounds)
: mSounds (sounds)
{}

int CSMTools::SoundCheckStage::setup()
{
    return mSounds.getSize();
}

void CSMTools::SoundCheckStage::perform (int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<ESM::Sound>& record = mSounds.getRecord (stage);

    if (record.isDeleted())
        return;

    const ESM::Sound& sound = record.get();

    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Sound, sound.mId);

    if (sound.mData.mMinRange>sound.mData.mMaxRange)
        messages.push_back (std::make_pair (id, "Minimum range larger than maximum range"));

    /// \todo check, if the sound file exists
}
