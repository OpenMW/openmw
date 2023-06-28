#ifndef MWMECHANICS_SPELLUTIL_H
#define MWMECHANICS_SPELLUTIL_H

#include <components/esm3/loadskil.hpp>

namespace ESM
{
    struct ENAMstruct;
    struct Enchantment;
    struct MagicEffect;
    struct Spell;
}

namespace MWWorld
{
    class Ptr;
}

namespace MWMechanics
{
    enum class EffectCostMethod
    {
        GameSpell,
        PlayerSpell,
        GameEnchantment,
    };

    float calcEffectCost(const ESM::ENAMstruct& effect, const ESM::MagicEffect* magicEffect = nullptr,
        const EffectCostMethod method = EffectCostMethod::GameSpell);
    int calcSpellCost(const ESM::Spell& spell);

    int getEffectiveEnchantmentCastCost(float castCost, const MWWorld::Ptr& actor);
    int getEffectiveEnchantmentCastCost(const ESM::Enchantment& enchantment, const MWWorld::Ptr& actor);
    int getEnchantmentCharge(const ESM::Enchantment& enchantment);

    /**
     * @param spell spell to cast
     * @param actor calculate spell success chance for this actor (depends on actor's skills)
     * @param effectiveSchool the spell's effective school (relevant for skill progress) will be written here
     * @param cap cap the result to 100%?
     * @param checkMagicka check magicka?
     * @note actor can be an NPC or a creature
     * @return success chance from 0 to 100 (in percent), if cap=false then chance above 100 may be returned.
     */
    float calcSpellBaseSuccessChance(const ESM::Spell* spell, const MWWorld::Ptr& actor, ESM::RefId* effectiveSchool);
    float getSpellSuccessChance(const ESM::Spell* spell, const MWWorld::Ptr& actor,
        ESM::RefId* effectiveSchool = nullptr, bool cap = true, bool checkMagicka = true);
    float getSpellSuccessChance(const ESM::RefId& spellId, const MWWorld::Ptr& actor,
        ESM::RefId* effectiveSchool = nullptr, bool cap = true, bool checkMagicka = true);

    ESM::RefId getSpellSchool(const ESM::RefId& spellId, const MWWorld::Ptr& actor);
    ESM::RefId getSpellSchool(const ESM::Spell* spell, const MWWorld::Ptr& actor);

    /// Get whether or not the given spell contributes to skill progress.
    bool spellIncreasesSkill(const ESM::Spell* spell);
    bool spellIncreasesSkill(const ESM::RefId& spellId);
}

#endif
