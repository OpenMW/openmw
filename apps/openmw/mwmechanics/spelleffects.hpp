#ifndef GAME_MWMECHANICS_SPELLEFFECTS_H
#define GAME_MWMECHANICS_SPELLEFFECTS_H

#include "activespells.hpp"

// These functions should probably be split up into separate Lua functions for each magic effect when magic is
// dehardcoded. That way ESM::MGEF could point to two Lua scripts for each effect. Needs discussion.

namespace MWWorld
{
    class Ptr;
}

namespace MWMechanics
{
    struct MagicApplicationResult
    {
        enum class Type
        {
            APPLIED,
            REMOVED,
            REFLECTED
        };
        Type mType;
        bool mShowHit;
        bool mShowHealth;
    };

    // Applies a tick of a single effect. Returns true if the effect should be removed immediately
    MagicApplicationResult applyMagicEffect(const MWWorld::Ptr& target, const MWWorld::Ptr& caster,
        ActiveSpells::ActiveSpellParams& spellParams, ESM::ActiveEffect& effect, float dt,
        bool playNonLoopingEffect = true);

    // Undoes permanent effects created by ESM::MagicEffect::AppliedOnce
    void onMagicEffectRemoved(
        const MWWorld::Ptr& target, ActiveSpells::ActiveSpellParams& spell, const ESM::ActiveEffect& effect);
}

#endif
