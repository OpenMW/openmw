#ifndef MWMECHANICS_SPELLSUCCESS_H
#define MWMECHANICS_SPELLSUCCESS_H

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/class.hpp"
#include "../mwmechanics/creaturestats.hpp"

#include <components/esm_store/store.hpp>

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

    inline int getSpellSchool(const ESM::Spell* spell, const MWWorld::Ptr& actor)
    {
        NpcStats& stats = MWWorld::Class::get(actor).getNpcStats(actor);

        // determine the spell's school
        // this is always the school where the player's respective skill is the least advanced
        // out of all the magic effects' schools
        const std::vector<ESM::ENAMstruct>& effects = spell->mEffects.mList;
        int school = -1;
        int skillLevel = -1;
        for (std::vector<ESM::ENAMstruct>::const_iterator it = effects.begin();
            it != effects.end(); ++it)
        {
            const ESM::MagicEffect* effect = MWBase::Environment::get().getWorld()->getStore().magicEffects.find(it->mEffectID);
            int _school = effect->mData.mSchool;
            int _skillLevel = stats.getSkill (spellSchoolToSkill(_school)).getModified();

            if (school == -1)
            {
                school = _school;
                skillLevel = _skillLevel;
            }
            else if (_skillLevel < skillLevel)
            {
                school = _school;
                skillLevel = _skillLevel;
            }
        }

        return school;
    }

    inline int getSpellSchool(const std::string& spellId, const MWWorld::Ptr& actor)
    {
        const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().spells.find(spellId);
        return getSpellSchool(spell, actor);
    }


    // UESP wiki / Morrowind/Spells:
    // Chance of success is (Spell's skill * 2 + Willpower / 5 + Luck / 10 - Spell cost - Sound magnitude) * (Current fatigue + Maximum Fatigue * 1.5) / Maximum fatigue * 2
    /**
     * @param spell spell to cast
     * @param actor calculate spell success chance for this actor (depends on actor's skills)
     * @attention actor has to be an NPC and not a creature!
     * @return success chance from 0 to 100 (in percent)
     */
    inline float getSpellSuccessChance (const ESM::Spell* spell, const MWWorld::Ptr& actor)
    {
        if (spell->mData.mFlags & ESM::Spell::F_Always // spells with this flag always succeed (usually birthsign spells)
            || spell->mData.mType == ESM::Spell::ST_Power) // powers always succeed, but can be cast only once per day
            return 100.0;

        if (spell->mEffects.mList.size() == 0)
            return 0.0;

        NpcStats& stats = MWWorld::Class::get(actor).getNpcStats(actor);
        CreatureStats& creatureStats = MWWorld::Class::get(actor).getCreatureStats(actor);

        int skillLevel = stats.getSkill (getSpellSchool(spell, actor)).getModified();

        // Sound magic effect (reduces spell casting chance)
        int soundMagnitude = creatureStats.getMagicEffects().get (MWMechanics::EffectKey (48)).mMagnitude;

        int willpower = creatureStats.getAttribute(ESM::Attribute::Willpower).getModified();
        int luck = creatureStats.getAttribute(ESM::Attribute::Luck).getModified();
        int currentFatigue = creatureStats.getFatigue().getCurrent();
        int maxFatigue = creatureStats.getFatigue().getModified();
        int spellCost = spell->mData.mCost;

        // There we go, all needed variables are there, lets go
        float chance = (float(skillLevel * 2) + float(willpower)/5.0 + float(luck)/ 10.0 - spellCost - soundMagnitude) * (float(currentFatigue + maxFatigue * 1.5)) / float(maxFatigue * 2.0);

        chance = std::max(0.0f, std::min(100.0f, chance)); // clamp to 0 .. 100

        return chance;
    }

    inline float getSpellSuccessChance (const std::string& spellId, const MWWorld::Ptr& actor)
    {
        const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().spells.find(spellId);
        return getSpellSuccessChance(spell, actor);
    }
}

#endif
