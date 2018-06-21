#include "soundgencheck.hpp"

#include <sstream>

#include "../prefs/state.hpp"

#include "../world/refiddata.hpp"
#include "../world/universalid.hpp"

CSMTools::SoundGenCheckStage::SoundGenCheckStage(const CSMWorld::IdCollection<ESM::SoundGenerator> &soundGens,
                                                 const CSMWorld::IdCollection<ESM::Sound> &sounds,
                                                 const CSMWorld::RefIdCollection &referenceables)
    : mSoundGens(soundGens),
      mSounds(sounds),
      mReferenceables(referenceables)
{
    mIgnoreBaseRecords = false;
}

int CSMTools::SoundGenCheckStage::setup()
{
    mIgnoreBaseRecords = CSMPrefs::get()["Reports"]["ignore-base-records"].isTrue();

    return mSoundGens.getSize();
}

void CSMTools::SoundGenCheckStage::perform(int stage, CSMDoc::Messages &messages)
{
    const CSMWorld::Record<ESM::SoundGenerator> &record = mSoundGens.getRecord(stage);
    
    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && record.mState == CSMWorld::RecordBase::State_BaseOnly) || record.isDeleted())
        return;

    const ESM::SoundGenerator& soundGen = record.get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_SoundGen, soundGen.mId);

    if (!soundGen.mCreature.empty())
    {
        CSMWorld::RefIdData::LocalIndex creatureIndex = mReferenceables.getDataSet().searchId(soundGen.mCreature);
        if (creatureIndex.first == -1)
        {
            messages.push_back(std::make_pair(id, "No such creature '" + soundGen.mCreature + "'"));
        }
        else if (creatureIndex.second != CSMWorld::UniversalId::Type_Creature)
        {
            messages.push_back(std::make_pair(id, "'" + soundGen.mCreature + "' is not a creature"));
        }
    }

    if (soundGen.mSound.empty())
    {
        messages.push_back(std::make_pair(id, "Sound is not specified"));
    }
    else if (mSounds.searchId(soundGen.mSound) == -1)
    {
        messages.push_back(std::make_pair(id, "No such sound '" + soundGen.mSound + "'"));
    }
}
