#ifndef GAME_MWMECHANICS_SPELLEFFECTS_H
#define GAME_MWMECHANICS_SPELLEFFECTS_H

#include "activespells.hpp"

#include "../mwworld/ptr.hpp"

// These functions should probably be split up into separate Lua functions for each magic effect when magic is dehardcoded.
// That way ESM::MGEF could point to two Lua scripts for each effect. Needs discussion.

namespace MWMechanics
{
    enum class MagicApplicationResult
    {
        APPLIED, REMOVED, REFLECTED
    };

    // Applies a tick of a single effect. Returns true if the effect should be removed immediately
    MagicApplicationResult applyMagicEffect(const MWWorld::Ptr& target, const MWWorld::Ptr& caster, ActiveSpells::ActiveSpellParams& spellParams, ESM::ActiveEffect& effect, float dt);

    // Undoes permanent effects created by ESM::MagicEffect::AppliedOnce
    void onMagicEffectRemoved(const MWWorld::Ptr& target, ActiveSpells::ActiveSpellParams& spell, const ESM::ActiveEffect& effect);
}

#endif
