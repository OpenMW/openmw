#ifndef MWMECHANICS_SPELLCASTING_H
#define MWMECHANICS_SPELLCASTING_H

#include <components/esm3/activespells.hpp>
#include <components/esm3/effectlist.hpp>

#include "../mwworld/ptr.hpp"

namespace ESM
{
    struct Spell;
    struct Ingredient;
    struct Potion;
    struct EffectList;
    struct Enchantment;
    struct MagicEffect;
}

namespace MWMechanics
{
    struct EffectKey;

    class CastSpell
    {
    private:
        MWWorld::Ptr mCaster; // May be empty
        MWWorld::Ptr mTarget; // May be empty

        void playSpellCastingEffects(const std::vector<ESM::IndexedENAMstruct>& effects) const;

        void explodeSpell(const ESM::EffectList& effects, const MWWorld::Ptr& ignore, ESM::RangeType rangeType) const;

        /// Launch a bolt with the given effects.
        void launchMagicBolt() const;

    public:
        ESM::RefId mId; // ID of spell, potion, item etc
        std::string mSourceName; // Display name for spell, potion, etc
        osg::Vec3f mHitPosition{ 0, 0, 0 }; // Used for spawning area orb
        bool mAlwaysSucceed{
            false
        }; // Always succeed spells casted by NPCs/creatures regardless of their chance (default: false)
        bool mFromProjectile; // True if spell is cast by enchantment of some projectile (arrow, bolt or thrown weapon)
        bool mScriptedSpell; // True if spell is casted from script and ignores some checks (mana level, success chance,
                             // etc.)
        ESM::RefNum mItem;
        ESM::ActiveSpells::Flags mFlags{ ESM::ActiveSpells::Flag_Temporary };

        CastSpell(const MWWorld::Ptr& caster, const MWWorld::Ptr& target, const bool fromProjectile = false,
            const bool scriptedSpell = false);

        bool cast(const ESM::Spell* spell);

        /// @note mCaster must be an actor
        /// @param launchProjectile If set to false, "on target" effects are directly applied instead of being launched
        /// as projectile originating from the caster.
        bool cast(const MWWorld::Ptr& item, bool launchProjectile = true);

        /// @note mCaster must be an NPC
        bool cast(const ESM::Ingredient* ingredient);

        bool cast(const ESM::Potion* potion);

        /// @note Auto detects if spell, ingredient or potion
        bool cast(const ESM::RefId& id);

        void playSpellCastingEffects(const ESM::Enchantment* enchantment) const;

        void playSpellCastingEffects(const ESM::Spell* spell) const;

        /// @note \a target can be any type of object, not just actors.
        void inflict(const MWWorld::Ptr& target, const ESM::EffectList& effects, ESM::RangeType range,
            bool exploded = false) const;
    };

    void playEffects(const MWWorld::Ptr& target, const ESM::MagicEffect& magicEffect, bool playNonLooping = true);
}

#endif
