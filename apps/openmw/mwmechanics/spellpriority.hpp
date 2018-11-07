#ifndef OPENMW_SPELL_PRIORITY_H
#define OPENMW_SPELL_PRIORITY_H

#include <components/esm/loadspel.hpp>

#include "../mwworld/ptr.hpp"

namespace MWMechanics
{
    // RangeTypes using bitflags to allow multiple range types, as can be the case with spells having multiple effects.
    enum RangeTypes
    {
        Self = 0x1,
        Touch = 0x10,
        Target = 0x100
    };

    int getRangeTypes (const ESM::EffectList& effects);

    float rateSpell (const ESM::Spell* spell, const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy);
    float rateMagicItem (const MWWorld::Ptr& ptr, const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy);
    float ratePotion (const MWWorld::Ptr& item, const MWWorld::Ptr &actor);

    /// @note target may be empty
    /// @param autoCalcCost toggles effect cost autocalculation which is then applied to the rating
    /// @note autoCalcCost must be false if (and must only be false if) you apply a predetermined cost to the rating
    float rateEffect (const ESM::ENAMstruct& effect, const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy, bool autoCalcCost = false);
    /// @note target may be empty
    /// @param autoCalcCost toggles effect cost autocalculation in the rateEffect() calls that follow
    /// @note autoCalcCost must be false if (and must only be false if) you apply a predetermined cost to the rating
    float rateEffects (const ESM::EffectList& list, const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy, bool autoCalcCost = false);

    float vanillaRateSpell(const ESM::Spell* spell, const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy);
}

#endif
