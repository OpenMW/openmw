#include "esmloader.hpp"
#include "esmstore.hpp"

#include <components/esm/esmreader.hpp>
#include <components/esm/npcstate.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/magiceffects.hpp"

namespace
{
    template<class T>
    void getEnchantedItem(const std::string& id, std::string& enchantment, std::string& itemName)
    {
        const T* item = MWBase::Environment::get().getWorld()->getStore().get<T>().search(id);
        if(item)
        {
            enchantment = item->mEnchant;
            itemName = item->mName;
        }
    }
}

namespace MWWorld
{

EsmLoader::EsmLoader(MWWorld::ESMStore& store, std::vector<ESM::ESMReader>& readers,
  ToUTF8::Utf8Encoder* encoder, Loading::Listener& listener)
  : ContentLoader(listener)
  , mEsm(readers)
  , mStore(store)
  , mEncoder(encoder)
{
}

void EsmLoader::load(const boost::filesystem::path& filepath, int& index)
{
  ContentLoader::load(filepath.filename(), index);

  ESM::ESMReader lEsm;
  lEsm.setEncoder(mEncoder);
  lEsm.setIndex(index);
  lEsm.open(filepath.string());
  lEsm.resolveParentFileIndices(mEsm);
  mEsm[index] = lEsm;
  mStore.load(mEsm[index], &mListener);
}

    void convertMagicEffects(ESM::CreatureStats& creatureStats, ESM::InventoryState& inventory, ESM::NpcStats* npcStats)
    {
        const auto& store = MWBase::Environment::get().getWorld()->getStore();
        // Convert corprus to format 10
        for (const auto& [id, oldStats] : creatureStats.mSpells.mCorprusSpells)
        {
            const ESM::Spell* spell = store.get<ESM::Spell>().search(id);
            if (!spell)
                continue;

            ESM::CreatureStats::CorprusStats stats;
            stats.mNextWorsening = oldStats.mNextWorsening;
            for (int i=0; i<ESM::Attribute::Length; ++i)
                stats.mWorsenings[i] = 0;

            for (auto& effect : spell->mEffects.mList)
            {
                if (effect.mEffectID == ESM::MagicEffect::DrainAttribute)
                    stats.mWorsenings[effect.mAttribute] = oldStats.mWorsenings;
            }
            creatureStats.mCorprusSpells[id] = stats;
        }
        // Convert to format 17
        for(const auto& [id, oldParams] : creatureStats.mSpells.mSpellParams)
        {
            const ESM::Spell* spell = store.get<ESM::Spell>().search(id);
            if (!spell || spell->mData.mType == ESM::Spell::ST_Spell || spell->mData.mType == ESM::Spell::ST_Power)
                continue;
            ESM::ActiveSpells::ActiveSpellParams params;
            params.mId = id;
            params.mDisplayName = spell->mName;
            params.mItem.unset();
            params.mCasterActorId = creatureStats.mActorId;
            if(spell->mData.mType == ESM::Spell::ST_Ability)
                params.mType = ESM::ActiveSpells::Type_Ability;
            else
                params.mType = ESM::ActiveSpells::Type_Permanent;
            params.mWorsenings = -1;
            int effectIndex = 0;
            for(const auto& enam : spell->mEffects.mList)
            {
                if(oldParams.mPurgedEffects.find(effectIndex) == oldParams.mPurgedEffects.end())
                {
                    ESM::ActiveEffect effect;
                    effect.mEffectId = enam.mEffectID;
                    effect.mArg = MWMechanics::EffectKey(enam).mArg;
                    effect.mDuration = -1;
                    effect.mTimeLeft = -1;
                    effect.mEffectIndex = effectIndex;
                    auto rand = oldParams.mEffectRands.find(effectIndex);
                    if(rand != oldParams.mEffectRands.end())
                    {
                        float magnitude = (enam.mMagnMax - enam.mMagnMin) * rand->second + enam.mMagnMin;
                        effect.mMagnitude = magnitude;
                        effect.mMinMagnitude = magnitude;
                        effect.mMaxMagnitude = magnitude;
                        // Prevent recalculation of resistances and don't reflect or absorb the effect
                        effect.mFlags = ESM::ActiveEffect::Flag_Ignore_Resistances | ESM::ActiveEffect::Flag_Ignore_Reflect | ESM::ActiveEffect::Flag_Ignore_SpellAbsorption;
                    }
                    else
                    {
                        effect.mMagnitude = 0.f;
                        effect.mMinMagnitude = enam.mMagnMin;
                        effect.mMaxMagnitude = enam.mMagnMax;
                        effect.mFlags = ESM::ActiveEffect::Flag_None;
                    }
                    params.mEffects.emplace_back(effect);
                }
                effectIndex++;
            }
            creatureStats.mActiveSpells.mSpells.emplace_back(params);
        }
        std::multimap<std::string, int> equippedItems;
        for(std::size_t i = 0; i < inventory.mItems.size(); ++i)
        {
            const ESM::ObjectState& item = inventory.mItems[i];
            auto slot = inventory.mEquipmentSlots.find(i);
            if(slot != inventory.mEquipmentSlots.end())
                equippedItems.emplace(item.mRef.mRefID, slot->second);
        }
        for(const auto& [id, oldMagnitudes] : inventory.mPermanentMagicEffectMagnitudes)
        {
            std::string eId;
            std::string name;
            switch(store.find(id))
            {
                case ESM::REC_ARMO:
                    getEnchantedItem<ESM::Armor>(id, eId, name);
                    break;
                case ESM::REC_CLOT:
                    getEnchantedItem<ESM::Clothing>(id, eId, name);
                    break;
                case ESM::REC_WEAP:
                    getEnchantedItem<ESM::Weapon>(id, eId, name);
                    break;
            }
            if(eId.empty())
                continue;
            const ESM::Enchantment* enchantment = store.get<ESM::Enchantment>().search(eId);
            if(!enchantment)
                continue;
            ESM::ActiveSpells::ActiveSpellParams params;
            params.mId = id;
            params.mDisplayName = name;
            params.mCasterActorId = creatureStats.mActorId;
            params.mType = ESM::ActiveSpells::Type_Enchantment;
            params.mWorsenings = -1;
            for(std::size_t effectIndex = 0; effectIndex < oldMagnitudes.size() && effectIndex < enchantment->mEffects.mList.size(); ++effectIndex)
            {
                const auto& enam = enchantment->mEffects.mList[effectIndex];
                auto [random, multiplier] = oldMagnitudes[effectIndex];
                float magnitude = (enam.mMagnMax - enam.mMagnMin) * random + enam.mMagnMin;
                magnitude *= multiplier;
                if(magnitude <= 0)
                    continue;
                ESM::ActiveEffect effect;
                effect.mEffectId = enam.mEffectID;
                effect.mMagnitude = magnitude;
                effect.mMinMagnitude = magnitude;
                effect.mMaxMagnitude = magnitude;
                effect.mArg = MWMechanics::EffectKey(enam).mArg;
                effect.mDuration = -1;
                effect.mTimeLeft = -1;
                effect.mEffectIndex = static_cast<int>(effectIndex);
                // Prevent recalculation of resistances and don't reflect or absorb the effect
                effect.mFlags = ESM::ActiveEffect::Flag_Ignore_Resistances | ESM::ActiveEffect::Flag_Ignore_Reflect | ESM::ActiveEffect::Flag_Ignore_SpellAbsorption;
                params.mEffects.emplace_back(effect);
            }
            auto [begin, end] = equippedItems.equal_range(id);
            for(auto it = begin; it != end; ++it)
            {
                params.mItem = { static_cast<unsigned int>(it->second), 0 };
                creatureStats.mActiveSpells.mSpells.emplace_back(params);
            }
        }
        for(const auto& spell : creatureStats.mCorprusSpells)
        {
            auto it = std::find_if(creatureStats.mActiveSpells.mSpells.begin(), creatureStats.mActiveSpells.mSpells.end(), [&] (const auto& params) { return params.mId == spell.first; });
            if(it != creatureStats.mActiveSpells.mSpells.end())
            {
                it->mNextWorsening = spell.second.mNextWorsening;
                int worsenings = 0;
                for(int i = 0; i < ESM::Attribute::Length; ++i)
                    worsenings = std::max(spell.second.mWorsenings[i], worsenings);
                it->mWorsenings = worsenings;
            }
        }
        for(const auto& [key, actorId] : creatureStats.mSummonedCreatureMap)
        {
            if(actorId == -1)
                continue;
            for(auto& params : creatureStats.mActiveSpells.mSpells)
            {
                if(params.mId == key.mSourceId)
                {
                    bool found = false;
                    for(auto& effect : params.mEffects)
                    {
                        if(effect.mEffectId == key.mEffectId && effect.mEffectIndex == key.mEffectIndex)
                        {
                            effect.mArg = actorId;
                            found = true;
                            break;
                        }
                    }
                    if(found)
                        break;
                }
            }
        }
        // Reset modifiers that were previously recalculated each frame
        for(std::size_t i = 0; i < ESM::Attribute::Length; ++i)
            creatureStats.mAttributes[i].mMod = 0.f;
        for(std::size_t i = 0; i < 3; ++i)
            creatureStats.mDynamic[i].mMod = 0.f;
        for(std::size_t i = 0; i < 4; ++i)
            creatureStats.mAiSettings[i].mMod = 0.f;
        if(npcStats)
        {
            for(std::size_t i = 0; i < ESM::Skill::Length; ++i)
                npcStats->mSkills[i].mMod = 0.f;
        }
    }
} /* namespace MWWorld */
