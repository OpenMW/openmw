#include "soundgencheck.hpp"

#include <string>

#include "../prefs/state.hpp"

#include "../world/refiddata.hpp"
#include "../world/universalid.hpp"

#include <apps/opencs/model/doc/messages.hpp>
#include <apps/opencs/model/prefs/category.hpp>
#include <apps/opencs/model/prefs/setting.hpp>
#include <apps/opencs/model/world/idcollection.hpp>
#include <apps/opencs/model/world/record.hpp>
#include <apps/opencs/model/world/refidcollection.hpp>

#include <components/esm3/loadsndg.hpp>
#include <components/esm3/loadsoun.hpp>

CSMTools::SoundGenCheckStage::SoundGenCheckStage(const CSMWorld::IdCollection<ESM::SoundGenerator>& soundGens,
    const CSMWorld::IdCollection<ESM::Sound>& sounds, const CSMWorld::RefIdCollection& objects)
    : mSoundGens(soundGens)
    , mSounds(sounds)
    , mObjects(objects)
{
    mIgnoreBaseRecords = false;
}

int CSMTools::SoundGenCheckStage::setup()
{
    mIgnoreBaseRecords = CSMPrefs::get()["Reports"]["ignore-base-records"].isTrue();

    return mSoundGens.getSize();
}

void CSMTools::SoundGenCheckStage::perform(int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<ESM::SoundGenerator>& record = mSoundGens.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && record.mState == CSMWorld::RecordBase::State_BaseOnly) || record.isDeleted())
        return;

    const ESM::SoundGenerator& soundGen = record.get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_SoundGen, soundGen.mId);

    if (!soundGen.mCreature.empty())
    {
        CSMWorld::RefIdData::LocalIndex creatureIndex = mObjects.getDataSet().searchId(soundGen.mCreature);
        if (creatureIndex.first == -1)
        {
            messages.add(id, "Creature '" + soundGen.mCreature.getRefIdString() + "' doesn't exist", "",
                CSMDoc::Message::Severity_Error);
        }
        else if (creatureIndex.second != CSMWorld::UniversalId::Type_Creature)
        {
            messages.add(id, "'" + soundGen.mCreature.getRefIdString() + "' is not a creature", "",
                CSMDoc::Message::Severity_Error);
        }
    }

    if (soundGen.mSound.empty())
    {
        messages.add(id, "Sound is missing", "", CSMDoc::Message::Severity_Error);
    }
    else if (mSounds.searchId(soundGen.mSound) == -1)
    {
        messages.add(
            id, "Sound '" + soundGen.mSound.getRefIdString() + "' doesn't exist", "", CSMDoc::Message::Severity_Error);
    }
}
