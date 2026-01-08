#include "magiceffects.hpp"
#include "esmstore.hpp"

#include <components/esm3/loadarmo.hpp>
#include <components/esm3/loadclot.hpp>
#include <components/esm3/loadench.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadskil.hpp>
#include <components/esm3/loadspel.hpp>
#include <components/esm3/loadweap.hpp>
#include <components/esm3/npcstate.hpp>

#include "../mwbase/environment.hpp"
#include "../mwworld/worldmodel.hpp"

#include "../mwmechanics/magiceffects.hpp"

namespace
{
    template <class T>
    void getEnchantedItem(const ESM::RefId& id, ESM::RefId& enchantment, std::string& itemName)
    {
        const T* item = MWBase::Environment::get().getESMStore()->get<T>().search(id);
        if (item)
        {
            enchantment = item->mEnchant;
            itemName = item->mName;
        }
    }
}

namespace MWWorld
{
    void convertMagicEffects(ESM::CreatureStats& creatureStats, ESM::InventoryState& inventory, ESM::NpcStats* npcStats)
    {
        const auto& store = *MWBase::Environment::get().getESMStore();
        // Convert corprus to format 10
        for (const auto& [id, oldStats] : creatureStats.mSpells.mCorprusSpells)
        {
            const ESM::Spell* spell = store.get<ESM::Spell>().search(id);
            if (!spell)
                continue;

            ESM::CreatureStats::CorprusStats stats;
            stats.mNextWorsening = oldStats.mNextWorsening;
            stats.mWorsenings.fill(0);

            for (auto& effect : spell->mEffects.mList)
            {
                if (effect.mData.mEffectID == ESM::MagicEffect::DrainAttribute)
                    stats.mWorsenings[effect.mData.mAttribute] = oldStats.mWorsenings;
            }
            creatureStats.mCorprusSpells[id] = stats;
        }
        // Convert to format 17
        for (const auto& [id, oldParams] : creatureStats.mSpells.mSpellParams)
        {
            const ESM::Spell* spell = store.get<ESM::Spell>().search(id);
            if (!spell || spell->mData.mType == ESM::Spell::ST_Spell || spell->mData.mType == ESM::Spell::ST_Power)
                continue;
            ESM::ActiveSpells::ActiveSpellParams params;
            params.mSourceSpellId = id;
            params.mDisplayName = spell->mName;
            params.mCaster.mIndex = creatureStats.mActorId;
            if (spell->mData.mType == ESM::Spell::ST_Ability)
                params.mFlags = ESM::Compatibility::ActiveSpells::Type_Ability_Flags;
            else
                params.mFlags = ESM::Compatibility::ActiveSpells::Type_Permanent_Flags;
            params.mWorsenings = -1;
            params.mNextWorsening = ESM::TimeStamp();
            for (const auto& enam : spell->mEffects.mList)
            {
                if (oldParams.mPurgedEffects.find(enam.mIndex) == oldParams.mPurgedEffects.end())
                {
                    ESM::ActiveEffect effect;
                    effect.mEffectId = enam.mData.mEffectID;
                    effect.mArg = MWMechanics::EffectKey(enam.mData).mArg;
                    effect.mDuration = -1;
                    effect.mTimeLeft = -1;
                    effect.mEffectIndex = enam.mIndex;
                    auto rand = oldParams.mEffectRands.find(enam.mIndex);
                    if (rand != oldParams.mEffectRands.end())
                    {
                        float magnitude
                            = (enam.mData.mMagnMax - enam.mData.mMagnMin) * rand->second + enam.mData.mMagnMin;
                        effect.mMagnitude = magnitude;
                        effect.mMinMagnitude = magnitude;
                        effect.mMaxMagnitude = magnitude;
                        // Prevent recalculation of resistances and don't reflect or absorb the effect
                        effect.mFlags = ESM::ActiveEffect::Flag_Ignore_Resistances
                            | ESM::ActiveEffect::Flag_Ignore_Reflect | ESM::ActiveEffect::Flag_Ignore_SpellAbsorption;
                    }
                    else
                    {
                        effect.mMagnitude = 0.f;
                        effect.mMinMagnitude = static_cast<float>(enam.mData.mMagnMin);
                        effect.mMaxMagnitude = static_cast<float>(enam.mData.mMagnMax);
                        effect.mFlags = ESM::ActiveEffect::Flag_None;
                    }
                    params.mEffects.emplace_back(effect);
                }
            }
            creatureStats.mActiveSpells.mSpells.emplace_back(params);
        }
        std::multimap<ESM::RefId, ESM::RefNum> equippedItems;
        for (std::size_t i = 0; i < inventory.mItems.size(); ++i)
        {
            ESM::ObjectState& item = inventory.mItems[i];
            auto slot = inventory.mEquipmentSlots.find(static_cast<uint32_t>(i));
            if (slot != inventory.mEquipmentSlots.end())
            {
                MWBase::Environment::get().getWorldModel()->assignSaveFileRefNum(item.mRef);
                equippedItems.emplace(item.mRef.mRefID, item.mRef.mRefNum);
            }
        }
        for (const auto& [id, oldMagnitudes] : inventory.mPermanentMagicEffectMagnitudes)
        {
            ESM::RefId eId;
            std::string name;
            switch (store.find(id))
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
            if (eId.empty())
                continue;
            const ESM::Enchantment* enchantment = store.get<ESM::Enchantment>().search(eId);
            if (!enchantment)
                continue;
            ESM::ActiveSpells::ActiveSpellParams params;
            params.mSourceSpellId = id;
            params.mDisplayName = std::move(name);
            params.mCaster.mIndex = creatureStats.mActorId;
            params.mFlags = ESM::Compatibility::ActiveSpells::Type_Enchantment_Flags;
            params.mWorsenings = -1;
            params.mNextWorsening = ESM::TimeStamp();
            for (const auto& enam : enchantment->mEffects.mList)
            {
                auto [random, multiplier] = oldMagnitudes[enam.mIndex];
                float magnitude = (enam.mData.mMagnMax - enam.mData.mMagnMin) * random + enam.mData.mMagnMin;
                magnitude *= multiplier;
                if (magnitude <= 0)
                    continue;
                ESM::ActiveEffect effect;
                effect.mEffectId = enam.mData.mEffectID;
                effect.mMagnitude = magnitude;
                effect.mMinMagnitude = magnitude;
                effect.mMaxMagnitude = magnitude;
                effect.mArg = MWMechanics::EffectKey(enam.mData).mArg;
                effect.mDuration = -1;
                effect.mTimeLeft = -1;
                effect.mEffectIndex = enam.mIndex;
                // Prevent recalculation of resistances and don't reflect or absorb the effect
                effect.mFlags = ESM::ActiveEffect::Flag_Ignore_Resistances | ESM::ActiveEffect::Flag_Ignore_Reflect
                    | ESM::ActiveEffect::Flag_Ignore_SpellAbsorption;
                params.mEffects.emplace_back(effect);
            }
            auto [begin, end] = equippedItems.equal_range(id);
            for (auto it = begin; it != end; ++it)
            {
                params.mItem = it->second;
                creatureStats.mActiveSpells.mSpells.emplace_back(params);
            }
        }
        for (const auto& spell : creatureStats.mCorprusSpells)
        {
            auto it
                = std::find_if(creatureStats.mActiveSpells.mSpells.begin(), creatureStats.mActiveSpells.mSpells.end(),
                    [&](const auto& params) { return params.mSourceSpellId == spell.first; });
            if (it != creatureStats.mActiveSpells.mSpells.end())
            {
                it->mNextWorsening = spell.second.mNextWorsening;
                int worsenings = 0;
                for (const auto& worsening : spell.second.mWorsenings)
                    worsenings = std::max(worsening, worsenings);
                it->mWorsenings = worsenings;
            }
        }
        for (const auto& [key, actorId] : creatureStats.mSummonedCreatureMap)
        {
            if (actorId < 0)
                continue;
            for (auto& params : creatureStats.mActiveSpells.mSpells)
            {
                if (params.mSourceSpellId == key.mSourceId)
                {
                    bool found = false;
                    for (auto& effect : params.mEffects)
                    {
                        if (effect.mEffectId == key.mEffectId && effect.mEffectIndex == key.mEffectIndex)
                        {
                            effect.mArg = ESM::RefNum{ .mIndex = static_cast<uint32_t>(actorId), .mContentFile = -1 };
                            effect.mFlags |= ESM::ActiveEffect::Flag_Applied | ESM::ActiveEffect::Flag_Remove;
                            found = true;
                            break;
                        }
                    }
                    if (found)
                        break;
                }
            }
        }
        // Reset modifiers that were previously recalculated each frame
        for (auto& attribute : creatureStats.mAttributes)
            attribute.mMod = 0.f;
        for (auto& dynamic : creatureStats.mDynamic)
        {
            dynamic.mCurrent -= dynamic.mMod - dynamic.mBase;
            dynamic.mMod = 0.f;
        }
        for (auto& setting : creatureStats.mAiSettings)
            setting.mMod = 0;
        if (npcStats)
        {
            for (auto& skill : npcStats->mSkills)
                skill.mMod = 0.f;
        }
    }

    // Versions 17-19 wrote different modifiers to the savegame depending on whether the save had upgraded from a pre-17
    // version or not
    void convertStats(ESM::CreatureStats& creatureStats)
    {
        for (auto& dynamic : creatureStats.mDynamic)
            dynamic.mMod = 0.f;
        for (auto& setting : creatureStats.mAiSettings)
            setting.mMod = 0;
    }

    // Versions 17-27 wrote an equipment slot index to mItem
    void convertEnchantmentSlots(ESM::CreatureStats& creatureStats, ESM::InventoryState& inventory)
    {
        for (auto& activeSpell : creatureStats.mActiveSpells.mSpells)
        {
            if (!activeSpell.mItem.isSet())
                continue;
            if (activeSpell.mFlags & ESM::ActiveSpells::Flag_Equipment)
            {
                std::int64_t slotIndex = activeSpell.mItem.mIndex;
                auto slot = std::find_if(inventory.mEquipmentSlots.begin(), inventory.mEquipmentSlots.end(),
                    [=](const auto& entry) { return entry.second == slotIndex; });
                if (slot != inventory.mEquipmentSlots.end() && slot->first < inventory.mItems.size())
                {
                    ESM::CellRef& ref = inventory.mItems[slot->first].mRef;
                    MWBase::Environment::get().getWorldModel()->assignSaveFileRefNum(ref);
                    activeSpell.mItem = ref.mRefNum;
                    continue;
                }
            }
            activeSpell.mItem = {};
        }
    }
}
