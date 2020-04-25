#ifndef MWMECHANICS_SPELLSUCCESS_H
#define MWMECHANICS_SPELLSUCCESS_H

#include <components/esm/effectlist.hpp>
#include <components/esm/loadskil.hpp>
#include <components/esm/loadmgef.hpp>

#include "../mwworld/ptr.hpp"

#include "magiceffects.hpp"

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
    class CreatureStats;

    ESM::Skill::SkillEnum spellSchoolToSkill(int school);

    float calcEffectCost(const ESM::ENAMstruct& effect, const ESM::MagicEffect* magicEffect = nullptr);

    bool isSummoningEffect(int effectId);

    /**
     * @param spell spell to cast
     * @param actor calculate spell success chance for this actor (depends on actor's skills)
     * @param effectiveSchool the spell's effective school (relevant for skill progress) will be written here
     * @param cap cap the result to 100%?
     * @param checkMagicka check magicka?
     * @note actor can be an NPC or a creature
     * @return success chance from 0 to 100 (in percent), if cap=false then chance above 100 may be returned.
     */
    float getSpellSuccessChance (const ESM::Spell* spell, const MWWorld::Ptr& actor, int* effectiveSchool = nullptr, bool cap=true, bool checkMagicka=true);
    float getSpellSuccessChance (const std::string& spellId, const MWWorld::Ptr& actor, int* effectiveSchool = nullptr, bool cap=true, bool checkMagicka=true);

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
                               const ESM::Spell* spell = nullptr, const MagicEffects* effects = nullptr);

    /// Get an effect multiplier for applying an effect cast by the given actor in the given spell (optional).
    /// @return effect multiplier from 0 to 2.  (100% net resistance to 100% net weakness)
    /// @param effects Override the actor's current magicEffects. Useful if there are effects currently
    ///                being applied (but not applied yet) that should also be considered.
    float getEffectMultiplier(short effectId, const MWWorld::Ptr& actor, const MWWorld::Ptr& caster,
                              const ESM::Spell* spell = nullptr, const MagicEffects* effects = nullptr);

    bool checkEffectTarget (int effectId, const MWWorld::Ptr& target, const MWWorld::Ptr& caster, bool castByPlayer);

    int getEffectiveEnchantmentCastCost (float castCost, const MWWorld::Ptr& actor);
    float calcSpellBaseSuccessChance (const ESM::Spell* spell, const MWWorld::Ptr& actor, int* effectiveSchool);

    /// Apply a magic effect that is applied in tick intervals until its remaining time ends or it is removed
    /// @return Was the effect a tickable effect with a magnitude?
    bool effectTick(CreatureStats& creatureStats, const MWWorld::Ptr& actor, const MWMechanics::EffectKey& effectKey, float magnitude);

    std::string getSummonedCreature(int effectId);

    class CastSpell
    {
    private:
        MWWorld::Ptr mCaster; // May be empty
        MWWorld::Ptr mTarget; // May be empty

        void playSpellCastingEffects(const std::vector<ESM::ENAMstruct>& effects);

    public:
        bool mStack{false};
        std::string mId; // ID of spell, potion, item etc
        std::string mSourceName; // Display name for spell, potion, etc
        osg::Vec3f mHitPosition{0,0,0}; // Used for spawning area orb
        bool mAlwaysSucceed{false}; // Always succeed spells casted by NPCs/creatures regardless of their chance (default: false)
        bool mFromProjectile; // True if spell is cast by enchantment of some projectile (arrow, bolt or thrown weapon)
        bool mManualSpell; // True if spell is casted from script and ignores some checks (mana level, success chance, etc.)

    public:
        CastSpell(const MWWorld::Ptr& caster, const MWWorld::Ptr& target, const bool fromProjectile=false, const bool manualSpell=false);

        bool cast (const ESM::Spell* spell);

        /// @note mCaster must be an actor
        /// @param launchProjectile If set to false, "on target" effects are directly applied instead of being launched as projectile originating from the caster.
        bool cast (const MWWorld::Ptr& item, bool launchProjectile=true);

        /// @note mCaster must be an NPC
        bool cast (const ESM::Ingredient* ingredient);

        bool cast (const ESM::Potion* potion);

        /// @note Auto detects if spell, ingredient or potion
        bool cast (const std::string& id);

        void playSpellCastingEffects(const std::string &spellid, bool enchantment);

        bool spellIncreasesSkill();

        /// Launch a bolt with the given effects.
        void launchMagicBolt ();

        /// @note \a target can be any type of object, not just actors.
        /// @note \a caster can be any type of object, or even an empty object.
        void inflict (const MWWorld::Ptr& target, const MWWorld::Ptr& caster,
                      const ESM::EffectList& effects, ESM::RangeType range, bool reflected=false, bool exploded=false);

        /// @note \a caster can be any type of object, or even an empty object.
        /// @return was the target suitable for the effect?
        bool applyInstantEffect (const MWWorld::Ptr& target, const MWWorld::Ptr& caster, const MWMechanics::EffectKey& effect, float magnitude);
    };

    class ApplyLoopingParticlesVisitor : public EffectSourceVisitor
    {
    private:
        MWWorld::Ptr mActor;

    public:
        ApplyLoopingParticlesVisitor(const MWWorld::Ptr& actor)
            : mActor(actor)
        {
        }

        virtual void visit (MWMechanics::EffectKey key,
                            const std::string& /*sourceName*/, const std::string& /*sourceId*/, int /*casterActorId*/,
                            float /*magnitude*/, float /*remainingTime*/ = -1, float /*totalTime*/ = -1);
    };
}

#endif
