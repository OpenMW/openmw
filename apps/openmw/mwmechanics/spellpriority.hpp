#ifndef OPENMW_SPELL_PRIORITY_H
#define OPENMW_SPELL_PRIORITY_H

namespace ESM
{
    struct Spell;
    struct EffectList;
    struct ENAMstruct;
}

namespace MWWorld
{
    class Ptr;
}

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
    float rateEffect (const ESM::ENAMstruct& effect, const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy);
    /// @note target may be empty
    float rateEffects (const ESM::EffectList& list, const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy);

    float vanillaRateSpell(const ESM::Spell* spell, const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy);
}

#endif
