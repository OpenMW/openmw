#ifndef MWMECHANICS_SPELLUTIL_H
#define MWMECHANICS_SPELLUTIL_H

#include <components/esm/loadskil.hpp>

namespace ESM
{
    struct ENAMstruct;
    struct MagicEffect;
    struct Spell;
}

namespace MWWorld
{
    class Ptr;
}

namespace MWMechanics
{
    ESM::Skill::SkillEnum spellSchoolToSkill(int school);

    enum class EffectCostMethod {
        GameSpell,
        PlayerSpell,
    };

    float calcEffectCost(const ESM::ENAMstruct& effect, const ESM::MagicEffect* magicEffect = nullptr, const EffectCostMethod method = EffectCostMethod::GameSpell);
    int calcSpellCost (const ESM::Spell& spell);

    int getEffectiveEnchantmentCastCost (float castCost, const MWWorld::Ptr& actor);

    /**
     * @param spell spell to cast
     * @param actor calculate spell success chance for this actor (depends on actor's skills)
     * @param effectiveSchool the spell's effective school (relevant for skill progress) will be written here
     * @param cap cap the result to 100%?
     * @param checkMagicka check magicka?
     * @note actor can be an NPC or a creature
     * @return success chance from 0 to 100 (in percent), if cap=false then chance above 100 may be returned.
     */
    float calcSpellBaseSuccessChance (const ESM::Spell* spell, const MWWorld::Ptr& actor, int* effectiveSchool);
    float getSpellSuccessChance (const ESM::Spell* spell, const MWWorld::Ptr& actor, int* effectiveSchool = nullptr, bool cap=true, bool checkMagicka=true);
    float getSpellSuccessChance (const std::string& spellId, const MWWorld::Ptr& actor, int* effectiveSchool = nullptr, bool cap=true, bool checkMagicka=true);

    int getSpellSchool(const std::string& spellId, const MWWorld::Ptr& actor);
    int getSpellSchool(const ESM::Spell* spell, const MWWorld::Ptr& actor);

    /// Get whether or not the given spell contributes to skill progress.
    bool spellIncreasesSkill(const ESM::Spell* spell);
    bool spellIncreasesSkill(const std::string& spellId);

    /// Check if the given effect can be applied to the target. If \a castByPlayer, emits a message box on failure.
    bool checkEffectTarget (int effectId, const MWWorld::Ptr& target, const MWWorld::Ptr& caster, bool castByPlayer);
}

#endif
