#ifndef MWMECHANICS_SPELLCASTING_H
#define MWMECHANICS_SPELLCASTING_H

#include <components/esm/effectlist.hpp>

#include "../mwworld/ptr.hpp"

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
}

#endif
