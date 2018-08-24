#include "magiceffectcheck.hpp"

#include <components/misc/resourcehelpers.hpp>

#include "../prefs/state.hpp"

#include "../world/data.hpp"

namespace
{
    void addMessage(CSMDoc::Messages &messages, const CSMWorld::UniversalId &id, const std::string& text)
    {
        if (!text.empty())
        {
            messages.push_back(std::make_pair(id, text));
        }
    }
}

std::string CSMTools::MagicEffectCheckStage::checkTexture(const std::string &texture, bool isIcon) const
{
    if (texture.empty()) return (isIcon ? "Icon is missing" : std::string());

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
    std::string error;
    if (!id.empty())
    {
        CSMWorld::RefIdData::LocalIndex index = mObjects.getDataSet().searchId(id);
        if (index.first == -1)
        {
            error = column + " '" + id + "' " + "does not exist";
        }
        else if (index.second != type.getType())
        {
            error = column + " '" + id + "' " + "does not have " + type.getTypeName() + " type";
        }
    }
    return error;
}

std::string CSMTools::MagicEffectCheckStage::checkSound(const std::string &id, const std::string &column) const
{
    std::string error;
    if (!id.empty() && mSounds.searchId(id) == -1)
    {
        error = column + " '" + id + "' " + "does not exist";
    }
    return error;
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
        addMessage(messages, id, "Description is missing");
    }

    if (effect.mData.mBaseCost < 0.0f)
    {
        addMessage(messages, id, "Base cost is negative");
    }

    addMessage(messages, id, checkTexture(effect.mIcon, true));
    addMessage(messages, id, checkTexture(effect.mParticle, false));
    addMessage(messages, id, checkObject(effect.mCasting, CSMWorld::UniversalId::Type_Static, "Casting object"));
    addMessage(messages, id, checkObject(effect.mHit, CSMWorld::UniversalId::Type_Static, "Hit object"));
    addMessage(messages, id, checkObject(effect.mArea, CSMWorld::UniversalId::Type_Static, "Area object"));
    addMessage(messages, id, checkObject(effect.mBolt, CSMWorld::UniversalId::Type_Weapon, "Bolt object"));
    addMessage(messages, id, checkSound(effect.mCastSound, "Casting sound"));
    addMessage(messages, id, checkSound(effect.mHitSound, "Hit sound"));
    addMessage(messages, id, checkSound(effect.mAreaSound, "Area sound"));
    addMessage(messages, id, checkSound(effect.mBoltSound, "Bolt sound"));
}
