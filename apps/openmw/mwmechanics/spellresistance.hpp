#ifndef MWMECHANICS_SPELLRESISTANCE_H
#define MWMECHANICS_SPELLRESISTANCE_H

namespace ESM
{
    struct Spell;
}

namespace MWWorld
{
    class Ptr;
}

namespace MWMechanics
{
    class MagicEffects;

    /// Get an effect multiplier for applying an effect cast by the given actor in the given spell (optional).
    /// @return effect multiplier from 0 to 2.  (100% net resistance to 100% net weakness)
    /// @param effects Override the actor's current magicEffects. Useful if there are effects currently
    ///                being applied (but not applied yet) that should also be considered.
    float getEffectMultiplier(short effectId, const MWWorld::Ptr& actor, const MWWorld::Ptr& caster,
                              const ESM::Spell* spell = nullptr, const MagicEffects* effects = nullptr);

    /// Get the effective resistance against an effect casted by the given actor in the given spell (optional).
    /// @return >=100 for fully resisted. can also return negative value for damage amplification.
    /// @param effects Override the actor's current magicEffects. Useful if there are effects currently
    ///                being applied (but not applied yet) that should also be considered.
    float getEffectResistance (short effectId, const MWWorld::Ptr& actor, const MWWorld::Ptr& caster,
                               const ESM::Spell* spell = nullptr, const MagicEffects* effects = nullptr);

    /// Get the resistance attribute against an effect for a given actor. This will add together
    /// ResistX and Weakness to X effects relevant against the given effect.
    float getEffectResistanceAttribute (short effectId, const MagicEffects* actorEffects);
}

#endif
