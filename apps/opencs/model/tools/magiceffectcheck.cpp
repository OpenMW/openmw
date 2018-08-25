#include "magiceffectcheck.hpp"

#include <components/misc/resourcehelpers.hpp>

#include "../prefs/state.hpp"

std::string CSMTools::MagicEffectCheckStage::checkTexture(const std::string &texture, bool isIcon) const
{
    const CSMWorld::Resources &textures = isIcon ? mIcons : mTextures;
    if (textures.searchId(texture) != -1) return std::string();

    std::string ddsTexture = texture;
    if (Misc::ResourceHelpers::changeExtensionToDds(ddsTexture) && textures.searchId(ddsTexture) != -1) return std::string();

    return (isIcon ? "Icon '" : "Particle '") + texture + "' does not exist";
}

std::string CSMTools::MagicEffectCheckStage::checkObject(const std::string &id, 
                                                                const CSMWorld::UniversalId &type, 
                                                                const std::string &column) const
{
    CSMWorld::RefIdData::LocalIndex index = mObjects.getDataSet().searchId(id);
    if (index.first == -1) return (column + " '" + id + "' " + "does not exist");
    else if (index.second != type.getType()) return (column + " '" + id + "' " + "does not have " + type.getTypeName() + " type");
    return std::string();
}

std::string CSMTools::MagicEffectCheckStage::checkSound(const std::string &id, const std::string &column) const
{
    if (!id.empty() && mSounds.searchId(id) == -1) return (column + " '" + id + "' " + "does not exist");
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
        const std::string error = checkTexture(effect.mIcon, true);
        if (!error.empty())
            messages.add(id, error, "", CSMDoc::Message::Severity_Error);
    }

    if (!effect.mParticle.empty())
    {
        const std::string error = checkTexture(effect.mParticle, false);
        if (!error.empty())
            messages.add(id, error, "", CSMDoc::Message::Severity_Error);
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

    if (!effect.mCastSound.empty())
    {
        const std::string error = checkSound(effect.mCastSound, "Casting sound");
        if (!error.empty())
            messages.add(id, error, "", CSMDoc::Message::Severity_Error);
    }

    if (!effect.mHitSound.empty())
    {
        const std::string error = checkSound(effect.mHitSound, "Hit sound");
        if (!error.empty())
            messages.add(id, error, "", CSMDoc::Message::Severity_Error);
    }

    if (!effect.mAreaSound.empty())
    {
        const std::string error = checkSound(effect.mAreaSound, "Area sound");
        if (!error.empty())
            messages.add(id, error, "", CSMDoc::Message::Severity_Error);
    }

    if (!effect.mBoltSound.empty())
    {
        const std::string error = checkSound(effect.mBoltSound, "Bolt sound");
        if (!error.empty())
            messages.add(id, error, "", CSMDoc::Message::Severity_Error);
    }
}
