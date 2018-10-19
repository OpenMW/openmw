#include "magiceffectcheck.hpp"

#include <components/misc/resourcehelpers.hpp>

#include "../prefs/state.hpp"

std::string CSMTools::MagicEffectCheckStage::checkObject(const std::string &id, 
                                                                const CSMWorld::UniversalId &type, 
                                                                const std::string &column) const
{
    CSMWorld::RefIdData::LocalIndex index = mObjects.getDataSet().searchId(id);
    if (index.first == -1) 
        return (column + " '" + id + "' does not exist");
    else if (index.second != type.getType()) 
        return (column + " '" + id + "' does not have " + type.getTypeName() + " type");
    return std::string();
}

CSMTools::MagicEffectCheckStage::MagicEffectCheckStage(const CSMWorld::IdCollection<ESM::MagicEffect> &effects,
                                                       const CSMWorld::IdCollection<ESM::Sound> &sounds,
                                                       const CSMWorld::RefIdCollection &objects,
                                                       const CSMWorld::Resources &icons,
                                                       const CSMWorld::Resources &textures)
    : mMagicEffects(effects),
      mSounds(sounds),
      mObjects(objects),
      mIcons(icons),
      mTextures(textures)
{
    mIgnoreBaseRecords = false;
}

int CSMTools::MagicEffectCheckStage::setup()
{
    mIgnoreBaseRecords = CSMPrefs::get()["Reports"]["ignore-base-records"].isTrue();

    return mMagicEffects.getSize();
}

void CSMTools::MagicEffectCheckStage::perform(int stage, CSMDoc::Messages &messages)
{
    const CSMWorld::Record<ESM::MagicEffect> &record = mMagicEffects.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && record.mState == CSMWorld::RecordBase::State_BaseOnly) || record.isDeleted())
        return;

    ESM::MagicEffect effect = record.get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_MagicEffect, effect.mId);

    if (effect.mDescription.empty())
    {
        messages.add(id, "Description is missing", "", CSMDoc::Message::Severity_Warning);
    }

    if (effect.mData.mBaseCost < 0.0f)
    {
        messages.add(id, "Base cost is negative", "", CSMDoc::Message::Severity_Error);
    }

    if (effect.mIcon.empty())
    {
        messages.add(id, "Icon is missing", "", CSMDoc::Message::Severity_Error);
    }
    else
    {
        if (mIcons.searchId(effect.mIcon) == -1)
        {
            std::string ddsIcon = effect.mIcon;
            if (!(Misc::ResourceHelpers::changeExtensionToDds(ddsIcon) && mIcons.searchId(ddsIcon) != -1))
                messages.add(id, "Icon '" + effect.mIcon + "' does not exist", "", CSMDoc::Message::Severity_Error);
        }
    }

    if (!effect.mParticle.empty())
    {
        if (mTextures.searchId(effect.mParticle) == -1)
        {
            std::string ddsParticle = effect.mParticle;
            if (!(Misc::ResourceHelpers::changeExtensionToDds(ddsParticle) && mTextures.searchId(ddsParticle) != -1))
                messages.add(id, "Particle texture '" + effect.mParticle + "' does not exist", "", CSMDoc::Message::Severity_Error);
        }
    }

    if (!effect.mCasting.empty())
    {
        const std::string error = checkObject(effect.mCasting, CSMWorld::UniversalId::Type_Static, "Casting object");
        if (!error.empty())
            messages.add(id, error, "", CSMDoc::Message::Severity_Error);
    }

    if (!effect.mHit.empty())
    {
        const std::string error = checkObject(effect.mHit, CSMWorld::UniversalId::Type_Static, "Hit object");
        if (!error.empty())
            messages.add(id, error, "", CSMDoc::Message::Severity_Error);
    }

    if (!effect.mArea.empty())
    {
        const std::string error = checkObject(effect.mArea, CSMWorld::UniversalId::Type_Static, "Area object");
        if (!error.empty())
            messages.add(id, error, "", CSMDoc::Message::Severity_Error);
    }

    if (!effect.mBolt.empty())
    {
        const std::string error = checkObject(effect.mBolt, CSMWorld::UniversalId::Type_Weapon, "Bolt object");
        if (!error.empty())
            messages.add(id, error, "", CSMDoc::Message::Severity_Error);
    }

    if (!effect.mCastSound.empty() && mSounds.searchId(effect.mCastSound) == -1)
        messages.add(id, "Casting sound '" + effect.mCastSound + "' does not exist", "", CSMDoc::Message::Severity_Error);
    if (!effect.mHitSound.empty() && mSounds.searchId(effect.mHitSound) == -1)
        messages.add(id, "Hit sound '" + effect.mHitSound + "' does not exist", "", CSMDoc::Message::Severity_Error);
    if (!effect.mAreaSound.empty() && mSounds.searchId(effect.mAreaSound) == -1)
        messages.add(id, "Area sound '" + effect.mAreaSound + "' does not exist", "", CSMDoc::Message::Severity_Error);
    if (!effect.mBoltSound.empty() && mSounds.searchId(effect.mBoltSound) == -1)
        messages.add(id, "Bolt sound '" + effect.mBoltSound + "' does not exist", "", CSMDoc::Message::Severity_Error);
}
