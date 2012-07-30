#ifndef MWMECHANICS_SPELLSUCCESS_H
#define MWMECHANICS_SPELLSUCCESS_H

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/class.hpp"
#include "../mwmechanics/creaturestats.hpp"

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

    inline int getSpellSchool(const std::string& spellId, const MWWorld::Ptr& actor)
    {
        const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().spells.find(spellId);
        NpcStats& stats = MWWorld::Class::get(actor).getNpcStats(actor);

        // determine the spell's school
        // this is always the school where the player's respective skill is the least advanced
        // out of all the magic effects' schools
        const std::vector<ESM::ENAMstruct>& effects = spell->effects.list;
        int school = -1;
        int skillLevel = -1;
        for (std::vector<ESM::ENAMstruct>::const_iterator it = effects.begin();
            it != effects.end(); ++it)
        {
            const ESM::MagicEffect* effect = MWBase::Environment::get().getWorld()->getStore().magicEffects.find(it->effectID);
            int _school = effect->data.school;
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


    // UESP wiki / Morrowind/Spells:
    // Chance of success is (Spell's skill * 2 + Willpower / 5 + Luck / 10 - Spell cost - Sound magnitude) * (Current fatigue + Maximum Fatigue * 1.5) / Maximum fatigue * 2
    /**
     * @param spellId ID of spell
     * @param actor calculate spell success chance for this actor (depends on actor's skills)
     * @attention actor has to be an NPC and not a creature!
     * @return success chance from 0 to 100 (in percent)
     */
    inline float getSpellSuccessChance (const std::string& spellId, const MWWorld::Ptr& actor)
    {
        const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().spells.find(spellId);

        if (spell->data.flags & ESM::Spell::F_Always // spells with this flag always succeed (usually birthsign spells)
            || spell->data.type == ESM::Spell::ST_Power) // powers always succeed, but can be cast only once per day
            return 100.0;

        NpcStats& stats = MWWorld::Class::get(actor).getNpcStats(actor);
        CreatureStats& creatureStats = MWWorld::Class::get(actor).getCreatureStats(actor);

        int skillLevel = stats.getSkill (getSpellSchool(spellId, actor)).getModified();

        // Sound magic effect (reduces spell casting chance)
        int soundMagnitude = creatureStats.getMagicEffects().get (MWMechanics::EffectKey (48)).mMagnitude;

        int willpower = creatureStats.getAttribute(ESM::Attribute::Willpower).getModified();
        int luck = creatureStats.getAttribute(ESM::Attribute::Luck).getModified();
        int currentFatigue = creatureStats.getFatigue().getCurrent();
        int maxFatigue = creatureStats.getFatigue().getModified();
        int spellCost = spell->data.cost;

        // There we go, all needed variables are there, lets go
        float chance = (float(skillLevel * 2) + float(willpower)/5.0 + float(luck)/ 10.0 - spellCost - soundMagnitude) * (float(currentFatigue + maxFatigue * 1.5)) / float(maxFatigue * 2.0);

        chance = std::max(0.0f, std::min(100.0f, chance)); // clamp to 0 .. 100

        return chance;
    }
}

#endif
