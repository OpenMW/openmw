#include "spellutil.hpp"

#include <limits>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include "actorutil.hpp"
#include "creaturestats.hpp"

namespace MWMechanics
{
    ESM::Skill::SkillEnum spellSchoolToSkill(int school)
    {
        static const std::array<ESM::Skill::SkillEnum, 6> schoolSkillArray
        {
            ESM::Skill::Alteration, ESM::Skill::Conjuration, ESM::Skill::Destruction,
            ESM::Skill::Illusion, ESM::Skill::Mysticism, ESM::Skill::Restoration
        };
        return schoolSkillArray.at(school);
    }

    float calcEffectCost(const ESM::ENAMstruct& effect, const ESM::MagicEffect* magicEffect, const EffectCostMethod method)
    {
        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
        if (!magicEffect)
            magicEffect = store.get<ESM::MagicEffect>().find(effect.mEffectID);
        bool hasMagnitude = !(magicEffect->mData.mFlags & ESM::MagicEffect::NoMagnitude);
        bool hasDuration = !(magicEffect->mData.mFlags & ESM::MagicEffect::NoDuration);
        bool appliedOnce = magicEffect->mData.mFlags & ESM::MagicEffect::AppliedOnce;
        int minMagn = hasMagnitude ? effect.mMagnMin : 1;
        int maxMagn = hasMagnitude ? effect.mMagnMax : 1;
        int duration = hasDuration ? effect.mDuration : 1;
        if (!appliedOnce)
            duration = std::max(1, duration);
        static const float fEffectCostMult = store.get<ESM::GameSetting>().find("fEffectCostMult")->mValue.getFloat();

        int durationOffset = 0;
        int minArea = 0;
        if (method == EffectCostMethod::PlayerSpell) {
            durationOffset = 1;
            minArea = 1;
        }

        float x = 0.5 * (std::max(1, minMagn) + std::max(1, maxMagn));
        x *= 0.1 * magicEffect->mData.mBaseCost;
        x *= durationOffset + duration;
        x += 0.05 * std::max(minArea, effect.mArea) * magicEffect->mData.mBaseCost;

        return x * fEffectCostMult;
    }

    int calcSpellCost (const ESM::Spell& spell)
    {
        if (!(spell.mData.mFlags & ESM::Spell::F_Autocalc))
            return spell.mData.mCost;

        float cost = 0;

        for (const ESM::ENAMstruct& effect : spell.mEffects.mList)
        {
            float effectCost = std::max(0.f, MWMechanics::calcEffectCost(effect));

            // This is applied to the whole spell cost for each effect when
            // creating spells, but is only applied on the effect itself in TES:CS.
            if (effect.mRange == ESM::RT_Target)
                effectCost *= 1.5;

            cost += effectCost;
        }

        return std::round(cost);
    }

    int getEffectiveEnchantmentCastCost(float castCost, const MWWorld::Ptr &actor)
    {
        /*
         * Each point of enchant skill above/under 10 subtracts/adds
         * one percent of enchantment cost while minimum is 1.
         */
        int eSkill = actor.getClass().getSkill(actor, ESM::Skill::Enchant);
        const float result = castCost - (castCost / 100) * (eSkill - 10);

        return static_cast<int>((result < 1) ? 1 : result);
    }

    float calcSpellBaseSuccessChance (const ESM::Spell* spell, const MWWorld::Ptr& actor, int* effectiveSchool)
    {
        // Morrowind for some reason uses a formula slightly different from magicka cost calculation
        float y = std::numeric_limits<float>::max();
        float lowestSkill = 0;

        for (const ESM::ENAMstruct& effect : spell->mEffects.mList)
        {
            float x = static_cast<float>(effect.mDuration);
            const auto magicEffect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(effect.mEffectID);

            if (!(magicEffect->mData.mFlags & ESM::MagicEffect::AppliedOnce))
                x = std::max(1.f, x);

            x *= 0.1f * magicEffect->mData.mBaseCost;
            x *= 0.5f * (effect.mMagnMin + effect.mMagnMax);
            x += effect.mArea * 0.05f * magicEffect->mData.mBaseCost;
            if (effect.mRange == ESM::RT_Target)
                x *= 1.5f;
            static const float fEffectCostMult = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(
                        "fEffectCostMult")->mValue.getFloat();
            x *= fEffectCostMult;

            float s = 2.0f * actor.getClass().getSkill(actor, spellSchoolToSkill(magicEffect->mData.mSchool));
            if (s - x < y)
            {
                y = s - x;
                if (effectiveSchool)
                    *effectiveSchool = magicEffect->mData.mSchool;
                lowestSkill = s;
            }
        }

        CreatureStats& stats = actor.getClass().getCreatureStats(actor);

        float actorWillpower = stats.getAttribute(ESM::Attribute::Willpower).getModified();
        float actorLuck = stats.getAttribute(ESM::Attribute::Luck).getModified();

        float castChance = (lowestSkill - calcSpellCost(*spell) + 0.2f * actorWillpower + 0.1f * actorLuck);

        return castChance;
    }

    float getSpellSuccessChance (const ESM::Spell* spell, const MWWorld::Ptr& actor, int* effectiveSchool, bool cap, bool checkMagicka)
    {
        // NB: Base chance is calculated here because the effective school pointer must be filled
        float baseChance = calcSpellBaseSuccessChance(spell, actor, effectiveSchool);

        bool godmode = actor == getPlayer() && MWBase::Environment::get().getWorld()->getGodModeState();

        CreatureStats& stats = actor.getClass().getCreatureStats(actor);

        if (stats.getMagicEffects().get(ESM::MagicEffect::Silence).getMagnitude() && !godmode)
            return 0;

        if (spell->mData.mType == ESM::Spell::ST_Power)
            return stats.getSpells().canUsePower(spell) ? 100 : 0;

        if (godmode)
            return 100;

        if (spell->mData.mType != ESM::Spell::ST_Spell)
            return 100;

        if (checkMagicka && calcSpellCost(*spell) > 0 && stats.getMagicka().getCurrent() < calcSpellCost(*spell))
            return 0;

        if (spell->mData.mFlags & ESM::Spell::F_Always)
            return 100;

        float castBonus = -stats.getMagicEffects().get(ESM::MagicEffect::Sound).getMagnitude();
        float castChance = baseChance + castBonus;
        castChance *= stats.getFatigueTerm();

        return std::max(0.f, cap ? std::min(100.f, castChance) : castChance);
    }

    float getSpellSuccessChance (const std::string& spellId, const MWWorld::Ptr& actor, int* effectiveSchool, bool cap, bool checkMagicka)
    {
        if (const auto spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(spellId))
            return getSpellSuccessChance(spell, actor, effectiveSchool, cap, checkMagicka);
        return 0.f;
    }

    int getSpellSchool(const std::string& spellId, const MWWorld::Ptr& actor)
    {
        int school = 0;
        getSpellSuccessChance(spellId, actor, &school);
        return school;
    }

    int getSpellSchool(const ESM::Spell* spell, const MWWorld::Ptr& actor)
    {
        int school = 0;
        getSpellSuccessChance(spell, actor, &school);
        return school;
    }

    bool spellIncreasesSkill(const ESM::Spell *spell)
    {
        return spell->mData.mType == ESM::Spell::ST_Spell && !(spell->mData.mFlags & ESM::Spell::F_Always);
    }

    bool spellIncreasesSkill(const std::string &spellId)
    {
        const auto spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(spellId);
        return spell && spellIncreasesSkill(spell);
    }

    bool checkEffectTarget (int effectId, const MWWorld::Ptr& target, const MWWorld::Ptr& caster, bool castByPlayer)
    {
        switch (effectId)
        {
            case ESM::MagicEffect::Levitate:
            {
                if (!MWBase::Environment::get().getWorld()->isLevitationEnabled())
                {
                    if (castByPlayer)
                        MWBase::Environment::get().getWindowManager()->messageBox("#{sLevitateDisabled}");
                    return false;
                }
                break;
            }
            case ESM::MagicEffect::Soultrap:
            {
                if (!target.getClass().isNpc() // no messagebox for NPCs
                     && (target.getTypeName() == typeid(ESM::Creature).name() && target.get<ESM::Creature>()->mBase->mData.mSoul == 0))
                {
                    if (castByPlayer)
                        MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicInvalidTarget}");
                    return true; // must still apply to get visual effect and have target regard it as attack
                }
                break;
            }
            case ESM::MagicEffect::WaterWalking:
            {
                if (target.getClass().isPureWaterCreature(target) && MWBase::Environment::get().getWorld()->isSwimming(target))
                    return false;

                MWBase::World *world = MWBase::Environment::get().getWorld();

                if (!world->isWaterWalkingCastableOnTarget(target))
                {
                    if (castByPlayer && caster == target)
                        MWBase::Environment::get().getWindowManager()->messageBox ("#{sMagicInvalidEffect}");
                    return false;
                }
                break;
            }
        }
        return true;
    }
}
