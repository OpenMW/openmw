#ifndef COMPONENTS_AUTOCALC_AUTOCALCSPELL_H
#define COMPONENTS_AUTOCALC_AUTOCALCSPELL_H

#include <vector>

#include <components/esm/loadskil.hpp>

namespace ESM
{
    struct Spell;
    struct Race;
}

namespace AutoCalc
{

class StoreCommon;

/// Contains algorithm for calculating an NPC's spells based on stats

std::vector<std::string> autoCalcNpcSpells(const int* actorSkills,
        const int* actorAttributes, const ESM::Race* race, StoreCommon *store);

// Helpers

bool attrSkillCheck (const ESM::Spell* spell,
        const int* actorSkills, const int* actorAttributes, StoreCommon *store);

ESM::Skill::SkillEnum mapSchoolToSkill(int school);

void calcWeakestSchool(const ESM::Spell* spell,
        const int* actorSkills, int& effectiveSchool, float& skillTerm, StoreCommon *store);

float calcAutoCastChance(const ESM::Spell* spell,
        const int* actorSkills, const int* actorAttributes, int effectiveSchool, StoreCommon *store);

}

#endif // COMPONENTS_AUTOCALC_AUTOCALCSPELL_H
