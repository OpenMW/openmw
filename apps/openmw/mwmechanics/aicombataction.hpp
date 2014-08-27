#ifndef OPENMW_AICOMBAT_ACTION_H
#define OPENMW_AICOMBAT_ACTION_H

#include <boost/shared_ptr.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/containerstore.hpp"

#include <components/esm/loadspel.hpp>

namespace MWMechanics
{

    class Action
    {
    public:
        virtual void prepare(const MWWorld::Ptr& actor) = 0;
        virtual void getCombatRange (float& rangeAttack, float& rangeFollow) = 0;
        virtual float getActionCooldown() { return 0.f; }
    };

    class ActionSpell : public Action
    {
    public:
        ActionSpell(const std::string& spellId) : mSpellId(spellId) {}
        std::string mSpellId;
        /// Sets the given spell as selected on the actor's spell list.
        virtual void prepare(const MWWorld::Ptr& actor);

        virtual void getCombatRange (float& rangeAttack, float& rangeFollow);
    };

    class ActionEnchantedItem : public Action
    {
    public:
        ActionEnchantedItem(const MWWorld::ContainerStoreIterator& item) : mItem(item) {}
        MWWorld::ContainerStoreIterator mItem;
        /// Sets the given item as selected enchanted item in the actor's InventoryStore.
        virtual void prepare(const MWWorld::Ptr& actor);
        virtual void getCombatRange (float& rangeAttack, float& rangeFollow);

        /// Since this action has no animation, apply a small cool down for using it
        virtual float getActionCooldown() { return 1.f; }
    };

    class ActionPotion : public Action
    {
    public:
        ActionPotion(const MWWorld::Ptr& potion) : mPotion(potion) {}
        MWWorld::Ptr mPotion;
        /// Drinks the given potion.
        virtual void prepare(const MWWorld::Ptr& actor);
        virtual void getCombatRange (float& rangeAttack, float& rangeFollow);

        /// Since this action has no animation, apply a small cool down for using it
        virtual float getActionCooldown() { return 1.f; }
    };

    class ActionWeapon : public Action
    {
    public:
        /// \a weapon may be empty for hand-to-hand combat
        ActionWeapon(const MWWorld::Ptr& weapon) : mWeapon(weapon) {}
        MWWorld::Ptr mWeapon;
        /// Equips the given weapon.
        virtual void prepare(const MWWorld::Ptr& actor);
        virtual void getCombatRange (float& rangeAttack, float& rangeFollow);
    };

    float rateSpell (const ESM::Spell* spell, const MWWorld::Ptr& actor, const MWWorld::Ptr& target);
    float rateMagicItem (const MWWorld::Ptr& ptr, const MWWorld::Ptr& actor, const MWWorld::Ptr &target);
    float ratePotion (const MWWorld::Ptr& item, const MWWorld::Ptr &actor);
    float rateWeapon (const MWWorld::Ptr& item, const MWWorld::Ptr& actor);

    /// @note target may be empty
    float rateEffect (const ESM::ENAMstruct& effect, const MWWorld::Ptr& actor, const MWWorld::Ptr& target);
    /// @note target may be empty
    float rateEffects (const ESM::EffectList& list, const MWWorld::Ptr& actor, const MWWorld::Ptr& target);

    boost::shared_ptr<Action> prepareNextAction (const MWWorld::Ptr& actor, const MWWorld::Ptr& target);
}

#endif
