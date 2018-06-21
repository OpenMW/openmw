#include "magiceffectcheck.hpp"

#include <components/misc/resourcehelpers.hpp>

#include "../prefs/state.hpp"

#include "../world/resources.hpp"
#include "../world/data.hpp"

namespace
{
    void addMessageIfNotEmpty(CSMDoc::Messages &messages, const CSMWorld::UniversalId &id, const std::string& text)
    {
        if (!text.empty())
        {
            messages.push_back(std::make_pair(id, text));
        }
    }
}

bool CSMTools::MagicEffectCheckStage::isTextureExists(const std::string &texture, bool isIcon) const
{
    const CSMWorld::Resources &textures = isIcon ? mIcons : mTextures;
    bool exists = false;

    if (textures.searchId(texture) != -1)
    {
        exists = true;
    }
    else
    {
        std::string ddsTexture = texture;
        if (Misc::ResourceHelpers::changeExtensionToDds(ddsTexture) && textures.searchId(ddsTexture) != -1)
        {
            exists = true;
        }
    }

    return exists;
}

std::string CSMTools::MagicEffectCheckStage::checkReferenceable(const std::string &id, 
                                                                const CSMWorld::UniversalId &type, 
                                                                const std::string &column) const
{
    std::string error;
    if (!id.empty())
    {
        CSMWorld::RefIdData::LocalIndex index = mReferenceables.getDataSet().searchId(id);
        if (index.first == -1)
        {
            error = "No such " + column + " '" + id + "'";
        }
        else if (index.second != type.getType())
        {
            error = column + " is not of type " + type.getTypeName();
        }
    }
    return error;
}

std::string CSMTools::MagicEffectCheckStage::checkSound(const std::string &id, const std::string &column) const
{
    std::string error;
    if (!id.empty() && mSounds.searchId(id) == -1)
    {
        error = "No such " + column + " '" + id + "'";
    }
    return error;
}

CSMTools::MagicEffectCheckStage::MagicEffectCheckStage(const CSMWorld::IdCollection<ESM::MagicEffect> &effects,
                                                       const CSMWorld::IdCollection<ESM::Sound> &sounds,
                                                       const CSMWorld::RefIdCollection &referenceables,
                                                       const CSMWorld::Resources &icons,
                                                       const CSMWorld::Resources &textures)
    : mMagicEffects(effects),
      mSounds(sounds),
      mReferenceables(referenceables),
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
    
    if (effect.mData.mBaseCost < 0.0f)
    {
        messages.push_back(std::make_pair(id, "Base Cost is negative"));
    }

    if (effect.mIcon.empty())
    {
        messages.push_back(std::make_pair(id, "Icon is not specified"));
    }
    else if (!isTextureExists(effect.mIcon, true))
    {
        messages.push_back(std::make_pair(id, "No such Icon '" + effect.mIcon + "'"));
    }

    if (!effect.mParticle.empty() && !isTextureExists(effect.mParticle, false))
    {
        messages.push_back(std::make_pair(id, "No such Particle '" + effect.mParticle + "'"));
    }

    addMessageIfNotEmpty(messages, 
                         id, 
                         checkReferenceable(effect.mCasting, CSMWorld::UniversalId::Type_Static, "Casting Object"));
    addMessageIfNotEmpty(messages, 
                         id,
                         checkReferenceable(effect.mHit, CSMWorld::UniversalId::Type_Static, "Hit Object"));
    addMessageIfNotEmpty(messages,
                         id,
                         checkReferenceable(effect.mArea, CSMWorld::UniversalId::Type_Static, "Area Object"));
    addMessageIfNotEmpty(messages,
                         id,
                         checkReferenceable(effect.mBolt, CSMWorld::UniversalId::Type_Weapon, "Bolt Object"));

    addMessageIfNotEmpty(messages, id, checkSound(effect.mCastSound, "Casting Sound"));
    addMessageIfNotEmpty(messages, id, checkSound(effect.mHitSound, "Hit Sound"));
    addMessageIfNotEmpty(messages, id, checkSound(effect.mAreaSound, "Area Sound"));
    addMessageIfNotEmpty(messages, id, checkSound(effect.mBoltSound, "Bolt Sound"));

    if (effect.mDescription.empty())
    {
        messages.push_back(std::make_pair(id, "Description is empty"));
    }
}
