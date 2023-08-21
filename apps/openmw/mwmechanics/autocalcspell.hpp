#ifndef OPENMW_AUTOCALCSPELL_H
#define OPENMW_AUTOCALCSPELL_H

#include "creaturestats.hpp"
#include <components/esm/refid.hpp>
#include <map>
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

    std::vector<ESM::RefId> autoCalcNpcSpells(const std::map<ESM::RefId, SkillValue>& actorSkills,
        const std::map<ESM::RefId, AttributeValue>& actorAttributes, const ESM::Race* race);

    std::vector<ESM::RefId> autoCalcPlayerSpells(const std::map<ESM::RefId, SkillValue>& actorSkills,
        const std::map<ESM::RefId, AttributeValue>& actorAttributes, const ESM::Race* race);

    // Helpers

    bool attrSkillCheck(const ESM::Spell* spell, const std::map<ESM::RefId, SkillValue>& actorSkills,
        const std::map<ESM::RefId, AttributeValue>& actorAttributes);

    void calcWeakestSchool(const ESM::Spell* spell, const std::map<ESM::RefId, SkillValue>& actorSkills,
        ESM::RefId& effectiveSchool, float& skillTerm);

    float calcAutoCastChance(const ESM::Spell* spell, const std::map<ESM::RefId, SkillValue>& actorSkills,
        const std::map<ESM::RefId, AttributeValue>& actorAttributes, ESM::RefId effectiveSchool);

}

#endif
