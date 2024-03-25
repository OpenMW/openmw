#ifndef MWMECHANICS_SPELLUTIL_H
#define MWMECHANICS_SPELLUTIL_H

#include <components/esm3/loadskil.hpp>

#include <optional>

namespace ESM
{
    struct EffectList;
    struct ENAMstruct;
    struct Enchantment;
    struct Ingredient;
    struct MagicEffect;
    struct Potion;
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
        GamePotion,
    };

    float calcEffectCost(const ESM::ENAMstruct& effect, const ESM::MagicEffect* magicEffect = nullptr,
        const EffectCostMethod method = EffectCostMethod::GameSpell);
    int calcSpellCost(const ESM::Spell& spell);

    int getEffectiveEnchantmentCastCost(float castCost, const MWWorld::Ptr& actor);
    int getEffectiveEnchantmentCastCost(const ESM::Enchantment& enchantment, const MWWorld::Ptr& actor);
    int getEnchantmentCharge(const ESM::Enchantment& enchantment);

    int getPotionValue(const ESM::Potion& potion);
    std::optional<ESM::EffectList> rollIngredientEffect(
        MWWorld::Ptr caster, const ESM::Ingredient* ingredient, uint32_t index = 0);

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
