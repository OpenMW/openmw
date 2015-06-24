#ifndef COMPONENTS_GAMEPLAY_AUTOCALCSPELL_H
#define COMPONENTS_GAMEPLAY_AUTOCALCSPELL_H

#include <vector>

#include <components/esm/loadskil.hpp>

namespace ESM
{
    struct Spell;
    struct Race;
}

namespace GamePlay
{

class StoreWrap;

/// Contains algorithm for calculating an NPC's spells based on stats

std::vector<std::string> autoCalcNpcSpells(const int* actorSkills,
        const int* actorAttributes, const ESM::Race* race, StoreWrap *store);

// Helpers

bool attrSkillCheck (const ESM::Spell* spell, const int* actorSkills, const int* actorAttributes, StoreWrap *store);

ESM::Skill::SkillEnum mapSchoolToSkill(int school);

void calcWeakestSchool(const ESM::Spell* spell,
        const int* actorSkills, int& effectiveSchool, float& skillTerm, StoreWrap *store);

float calcAutoCastChance(const ESM::Spell* spell,
        const int* actorSkills, const int* actorAttributes, int effectiveSchool, StoreWrap *store);

}

#endif
