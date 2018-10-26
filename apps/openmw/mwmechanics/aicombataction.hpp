#ifndef OPENMW_AICOMBAT_ACTION_H
#define OPENMW_AICOMBAT_ACTION_H

#include <memory>

#include <components/esm/loadspel.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/containerstore.hpp"

namespace MWMechanics
{
    class Action
    {
    public:
        virtual ~Action() {}
        virtual void prepare(const MWWorld::Ptr& actor) = 0;
        virtual float getCombatRange (bool& isRanged) const = 0;
        virtual float getActionCooldown() { return 0.f; }
        virtual const ESM::Weapon* getWeapon() const { return nullptr; };
        virtual bool isAttackingOrSpell() const { return true; }
        virtual bool isFleeing() const { return false; }
    };

    class ActionFlee : public Action
    {
    public:
        ActionFlee() {}
        virtual void prepare(const MWWorld::Ptr& actor) {}
        virtual float getCombatRange (bool& isRanged) const { return 0.0f; }
        virtual float getActionCooldown() { return 3.0f; }
        virtual bool isAttackingOrSpell() const { return false; }
        virtual bool isFleeing() const { return true; }
    };

    class ActionSpell : public Action
    {
    public:
        ActionSpell(const std::string& spellId) : mSpellId(spellId) {}
        std::string mSpellId;
        /// Sets the given spell as selected on the actor's spell list.
        virtual void prepare(const MWWorld::Ptr& actor);

        virtual float getCombatRange (bool& isRanged) const;
    };

    class ActionEnchantedItem : public Action
    {
    public:
        ActionEnchantedItem(const MWWorld::ContainerStoreIterator& item) : mItem(item) {}
        MWWorld::ContainerStoreIterator mItem;
        /// Sets the given item as selected enchanted item in the actor's InventoryStore.
        virtual void prepare(const MWWorld::Ptr& actor);
        virtual float getCombatRange (bool& isRanged) const;

        /// Since this action has no animation, apply a small cool down for using it
        virtual float getActionCooldown() { return 0.75f; }
    };

    class ActionPotion : public Action
    {
    public:
        ActionPotion(const MWWorld::Ptr& potion) : mPotion(potion) {}
        MWWorld::Ptr mPotion;
        /// Drinks the given potion.
        virtual void prepare(const MWWorld::Ptr& actor);
        virtual float getCombatRange (bool& isRanged) const;
        virtual bool isAttackingOrSpell() const { return false; }

        /// Since this action has no animation, apply a small cool down for using it
        virtual float getActionCooldown() { return 0.75f; }
    };

    class ActionWeapon : public Action
    {
    private:
        MWWorld::Ptr mAmmunition;
        MWWorld::Ptr mWeapon;

    public:
        /// \a weapon may be empty for hand-to-hand combat
        ActionWeapon(const MWWorld::Ptr& weapon, const MWWorld::Ptr& ammo = MWWorld::Ptr())
            : mAmmunition(ammo), mWeapon(weapon) {}
        /// Equips the given weapon.
        virtual void prepare(const MWWorld::Ptr& actor);
        virtual float getCombatRange (bool& isRanged) const;
        virtual const ESM::Weapon* getWeapon() const;
    };

    std::shared_ptr<Action> prepareNextAction (const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy);
    float getBestActionRating(const MWWorld::Ptr &actor, const MWWorld::Ptr &enemy);

    float getDistanceMinusHalfExtents(const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy, bool minusZDist=false);
    float getMaxAttackDistance(const MWWorld::Ptr& actor);
    bool canFight(const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy);

    float vanillaRateFlee(const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy);
    bool makeFleeDecision(const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy, float antiFleeRating);
}

#endif
