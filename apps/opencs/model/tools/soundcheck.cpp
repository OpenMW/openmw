
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

void CSMTools::SoundCheckStage::perform (int stage, std::vector<std::string>& messages)
{
    const ESM::Sound& sound = mSounds.getRecord (stage).get();

    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Sound, sound.mId);

    if (sound.mData.mMinRange>sound.mData.mMaxRange)
        messages.push_back (id.toString() + "|Maximum range larger than minimum range");

    /// \todo check, if the sound file exists
}