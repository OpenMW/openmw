#ifndef MWMECHANICS_SPELLSUCCESS_H
#define MWMECHANICS_SPELLSUCCESS_H

#include <cfloat>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/class.hpp"
#include "../mwmechanics/creaturestats.hpp"

#include "../mwworld/esmstore.hpp"

#include "npcstats.hpp"

namespace MWMechanics
{
    inline int spellSchoolToSkill(int school)
    {
        std::map<int, int> schoolSkillMap; // maps spell school to skill id
        schoolSkillMap[0] = 11; // alteration
        schoolSkillMap[1] = 13; // conjuration
        schoolSkillMap[3] = 12; // illusion
        schoolSkillMap[2] = 10; // destruction
        schoolSkillMap[4] = 14; // mysticism
        schoolSkillMap[5] = 15; // restoration
        assert(schoolSkillMap.find(school) != schoolSkillMap.end());
        return schoolSkillMap[school];
    }

    /**
     * @param spell spell to cast
     * @param actor calculate spell success chance for this actor (depends on actor's skills)
     * @param effectiveSchool the spell's effective school (relevant for skill progress) will be written here
     * @attention actor has to be an NPC and not a creature!
     * @return success chance from 0 to 100 (in percent)
     */
    inline float getSpellSuccessChance (const ESM::Spell* spell, const MWWorld::Ptr& actor, int* effectiveSchool = NULL)
    {
        NpcStats& stats = MWWorld::Class::get(actor).getNpcStats(actor);
        CreatureStats& creatureStats = MWWorld::Class::get(actor).getCreatureStats(actor);

        if (creatureStats.getMagicEffects().get(ESM::MagicEffect::Silence).mMagnitude)
            return 0;

        float y = FLT_MAX;
        float lowestSkill = 0;

        for (std::vector<ESM::ENAMstruct>::const_iterator it = spell->mEffects.mList.begin(); it != spell->mEffects.mList.end(); ++it)
        {
            float x = it->mDuration;
            const ESM::MagicEffect* magicEffect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(
                        it->mEffectID);
            if (!(magicEffect->mData.mFlags & ESM::MagicEffect::UncappedDamage))
                x = std::max(1.f, x);
            x *= 0.1 * magicEffect->mData.mBaseCost;
            x *= 0.5 * (it->mMagnMin + it->mMagnMax);
            x *= it->mArea * 0.05 * magicEffect->mData.mBaseCost;
            if (magicEffect->mData.mFlags & ESM::MagicEffect::CastTarget)
                x *= 1.5;
            static const float fEffectCostMult = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(
                        "fEffectCostMult")->getFloat();
            x *= fEffectCostMult;

            float s = 2 * stats.getSkill(spellSchoolToSkill(magicEffect->mData.mSchool)).getModified();
            if (s - x < y)
            {
                y = s - x;
                if (effectiveSchool)
                    *effectiveSchool = magicEffect->mData.mSchool;
                lowestSkill = s;
            }
        }

        if (spell->mData.mType != ESM::Spell::ST_Spell)
            return 100;

        if (spell->mData.mFlags & ESM::Spell::F_Always)
            return 100;

        int castBonus = -stats.getMagicEffects().get(ESM::MagicEffect::Sound).mMagnitude;

        int actorWillpower = stats.getAttribute(ESM::Attribute::Willpower).getModified();
        int actorLuck = stats.getAttribute(ESM::Attribute::Luck).getModified();

        float castChance = (lowestSkill - spell->mData.mCost + castBonus + 0.2 * actorWillpower + 0.1 * actorLuck) * stats.getFatigueTerm();
        if (MWBase::Environment::get().getWorld()->getGodModeState() && actor.getRefData().getHandle() == "player")
            castChance = 100;

        return std::max(0.f, std::min(100.f, castChance));
    }

    inline float getSpellSuccessChance (const std::string& spellId, const MWWorld::Ptr& actor, int* effectiveSchool = NULL)
    {
        const ESM::Spell* spell =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(spellId);
        return getSpellSuccessChance(spell, actor, effectiveSchool);
    }

    inline int getSpellSchool(const std::string& spellId, const MWWorld::Ptr& actor)
    {
        int school = 0;
        getSpellSuccessChance(spellId, actor, &school);
        return school;
    }

    inline int getSpellSchool(const ESM::Spell* spell, const MWWorld::Ptr& actor)
    {
        int school = 0;
        getSpellSuccessChance(spell, actor, &school);
        return school;
    }

    /// @return >=100 for fully resisted. can also return negative value for damage amplification.
    inline float getEffectResistance (short effectId, const MWWorld::Ptr& actor, const MWWorld::Ptr& caster, const ESM::Spell* spell = NULL)
    {
        const ESM::MagicEffect *magicEffect =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find (
            effectId);

        const MWMechanics::CreatureStats& stats = actor.getClass().getCreatureStats(actor);

        float resisted = 0;
        if (magicEffect->mData.mFlags & ESM::MagicEffect::Harmful)
        {

            short resistanceEffect = ESM::MagicEffect::getResistanceEffect(effectId);
            short weaknessEffect = ESM::MagicEffect::getWeaknessEffect(effectId);

            float resistance = 0;
            if (resistanceEffect != -1)
                resistance += stats.getMagicEffects().get(resistanceEffect).mMagnitude;
            if (weaknessEffect != -1)
                resistance -= stats.getMagicEffects().get(weaknessEffect).mMagnitude;


            float willpower = stats.getAttribute(ESM::Attribute::Willpower).getModified();
            float luck = stats.getAttribute(ESM::Attribute::Luck).getModified();
            float x = (willpower + 0.1 * luck) * stats.getFatigueTerm();

            // This makes spells that are easy to cast harder to resist and vice versa
            if (spell != NULL && caster.getClass().isActor())
            {
                float castChance = getSpellSuccessChance(spell, caster);
                if (castChance > 0)
                    x *= 50 / castChance;
            }

            float roll = static_cast<float>(std::rand()) / RAND_MAX * 100;
            if (magicEffect->mData.mFlags & ESM::MagicEffect::NoMagnitude)
                roll -= resistance;

            if (x <= roll)
                x = 0;
            else
            {
                if (magicEffect->mData.mFlags & ESM::MagicEffect::NoMagnitude)
                    x = 100;
                else
                    x = roll / std::min(x, 100.f);
            }

            x = std::min(x + resistance, 100.f);

            resisted = x;
        }

        return resisted;
    }

    inline float getEffectMultiplier(short effectId, const MWWorld::Ptr& actor, const MWWorld::Ptr& caster, const ESM::Spell* spell = NULL)
    {
        float resistance = getEffectResistance(effectId, actor, caster, spell);
        if (resistance >= 0)
            return 1 - resistance / 100.f;
        else
            return -(resistance-100) / 100.f;
    }


    class CastSpell
    {
    private:
        MWWorld::Ptr mCaster;
        MWWorld::Ptr mTarget;
    public:
        bool mStack;
        std::string mId; // ID of spell, potion, item etc
        std::string mSourceName; // Display name for spell, potion, etc

    public:
        CastSpell(const MWWorld::Ptr& caster, const MWWorld::Ptr& target);

        bool cast (const ESM::Spell* spell);
        bool cast (const MWWorld::Ptr& item);
        bool cast (const ESM::Ingredient* ingredient);
        bool cast (const ESM::Potion* potion);

        /// @note Auto detects if spell, ingredient or potion
        bool cast (const std::string& id);

        void inflict (const MWWorld::Ptr& target, const MWWorld::Ptr& caster,
                      const ESM::EffectList& effects, ESM::RangeType range, bool reflected=false);

        void applyInstantEffect (const MWWorld::Ptr& target, MWMechanics::EffectKey effect, float magnitude);
    };

}

#endif
