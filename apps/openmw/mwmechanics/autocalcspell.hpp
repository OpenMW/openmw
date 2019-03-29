#ifndef OPENMW_AUTOCALCSPELL_H
#define OPENMW_AUTOCALCSPELL_H

#include <string>
#include <vector>

namespace ESM
{
    struct Spell;
    struct Race;
}

namespace MWMechanics
{

/// Contains algorithm for calculating an NPC's spells based on stats
/// @note We might want to move this code to a component later, so the editor can use it for preview purposes

std::vector<std::string> autoCalcNpcSpells(const int* actorSkills, const int* actorAttributes, const ESM::Race* race);

std::vector<std::string> autoCalcPlayerSpells(const int* actorSkills, const int* actorAttributes, const ESM::Race* race);

// Helpers

bool attrSkillCheck (const ESM::Spell* spell, const int* actorSkills, const int* actorAttributes);

void calcWeakestSchool(const ESM::Spell* spell, const int* actorSkills, int& effectiveSchool, float& skillTerm);

float calcAutoCastChance(const ESM::Spell* spell, const int* actorSkills, const int* actorAttributes, int effectiveSchool);

}

#endif
