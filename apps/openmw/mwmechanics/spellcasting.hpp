#ifndef MWMECHANICS_SPELLSUCCESS_H
#define MWMECHANICS_SPELLSUCCESS_H

#include "../mwworld/ptr.hpp"

#include <OgreVector3.h>

#include <components/esm/loadskil.hpp>

namespace ESM
{
    struct Spell;
    struct Ingredient;
    struct Potion;
    struct EffectList;
}

namespace MWMechanics
{
    struct EffectKey;
    class MagicEffects;

    ESM::Skill::SkillEnum spellSchoolToSkill(int school);

    bool isSummoningEffect(int effectId);

    /**
     * @param spell spell to cast
     * @param actor calculate spell success chance for this actor (depends on actor's skills)
     * @param effectiveSchool the spell's effective school (relevant for skill progress) will be written here
     * @param cap cap the result to 100%?
     * @note actor can be an NPC or a creature
     * @return success chance from 0 to 100 (in percent), if cap=false then chance above 100 may be returned.
     */
    float getSpellSuccessChance (const ESM::Spell* spell, const MWWorld::Ptr& actor, int* effectiveSchool = NULL, bool cap=true);
    float getSpellSuccessChance (const std::string& spellId, const MWWorld::Ptr& actor, int* effectiveSchool = NULL, bool cap=true);

    int getSpellSchool(const std::string& spellId, const MWWorld::Ptr& actor);
    int getSpellSchool(const ESM::Spell* spell, const MWWorld::Ptr& actor);

    /// Get whether or not the given spell contributes to skill progress.
    bool spellIncreasesSkill(const ESM::Spell* spell);
    bool spellIncreasesSkill(const std::string& spellId);

    /// Get the resistance attribute against an effect for a given actor. This will add together
    /// ResistX and Weakness to X effects relevant against the given effect.
    float getEffectResistanceAttribute (short effectId, const MagicEffects* actorEffects);

    /// Get the effective resistance against an effect casted by the given actor in the given spell (optional).
    /// @return >=100 for fully resisted. can also return negative value for damage amplification.
    /// @param effects Override the actor's current magicEffects. Useful if there are effects currently
    ///                being applied (but not applied yet) that should also be considered.
    float getEffectResistance (short effectId, const MWWorld::Ptr& actor, const MWWorld::Ptr& caster,
                               const ESM::Spell* spell = NULL, const MagicEffects* effects = NULL);

    /// Get an effect multiplier for applying an effect cast by the given actor in the given spell (optional).
    /// @return effect multiplier from 0 to 2.  (100% net resistance to 100% net weakness)
    /// @param effects Override the actor's current magicEffects. Useful if there are effects currently
    ///                being applied (but not applied yet) that should also be considered.
    float getEffectMultiplier(short effectId, const MWWorld::Ptr& actor, const MWWorld::Ptr& caster,
                              const ESM::Spell* spell = NULL, const MagicEffects* effects = NULL);

    int getEffectiveEnchantmentCastCost (float castCost, const MWWorld::Ptr& actor);

    class CastSpell
    {
    private:
        MWWorld::Ptr mCaster; // May be empty
        MWWorld::Ptr mTarget; // May be empty
    public:
        bool mStack;
        std::string mId; // ID of spell, potion, item etc
        std::string mSourceName; // Display name for spell, potion, etc
        Ogre::Vector3 mHitPosition; // Used for spawning area orb
        bool mAlwaysSucceed; // Always succeed spells casted by NPCs/creatures regardless of their chance (default: false)

    public:
        CastSpell(const MWWorld::Ptr& caster, const MWWorld::Ptr& target);

        bool cast (const ESM::Spell* spell);

        /// @note mCaster must be an actor
        bool cast (const MWWorld::Ptr& item);

        /// @note mCaster must be an NPC
        bool cast (const ESM::Ingredient* ingredient);

        bool cast (const ESM::Potion* potion);

        /// @note Auto detects if spell, ingredient or potion
        bool cast (const std::string& id);

        /// @note \a target can be any type of object, not just actors.
        /// @note \a caster can be any type of object, or even an empty object.
        void inflict (const MWWorld::Ptr& target, const MWWorld::Ptr& caster,
                      const ESM::EffectList& effects, ESM::RangeType range, bool reflected=false, bool exploded=false);

        /// @note \a caster can be any type of object, or even an empty object.
        void applyInstantEffect (const MWWorld::Ptr& target, const MWWorld::Ptr& caster, const MWMechanics::EffectKey& effect, float magnitude);
    };

}

#endif
