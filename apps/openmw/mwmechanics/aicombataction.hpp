#ifndef OPENMW_AICOMBAT_ACTION_H
#define OPENMW_AICOMBAT_ACTION_H

#include <memory>

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
        virtual const ESM::Weapon* getWeapon() const { return nullptr; }
        virtual bool isAttackingOrSpell() const { return true; }
        virtual bool isFleeing() const { return false; }
    };

    class ActionFlee : public Action
    {
    public:
        ActionFlee() {}
        void prepare(const MWWorld::Ptr& actor) override {}
        float getCombatRange (bool& isRanged) const override { return 0.0f; }
        float getActionCooldown() override { return 3.0f; }
        bool isAttackingOrSpell() const override { return false; }
        bool isFleeing() const override { return true; }
    };

    class ActionSpell : public Action
    {
    public:
        ActionSpell(const std::string& spellId) : mSpellId(spellId) {}
        std::string mSpellId;
        /// Sets the given spell as selected on the actor's spell list.
        void prepare(const MWWorld::Ptr& actor) override;

        float getCombatRange (bool& isRanged) const override;
    };

    class ActionEnchantedItem : public Action
    {
    public:
        ActionEnchantedItem(const MWWorld::ContainerStoreIterator& item) : mItem(item) {}
        MWWorld::ContainerStoreIterator mItem;
        /// Sets the given item as selected enchanted item in the actor's InventoryStore.
        void prepare(const MWWorld::Ptr& actor) override;
        float getCombatRange (bool& isRanged) const override;

        /// Since this action has no animation, apply a small cool down for using it
        float getActionCooldown() override { return 0.75f; }
    };

    class ActionPotion : public Action
    {
    public:
        ActionPotion(const MWWorld::Ptr& potion) : mPotion(potion) {}
        MWWorld::Ptr mPotion;
        /// Drinks the given potion.
        void prepare(const MWWorld::Ptr& actor) override;
        float getCombatRange (bool& isRanged) const override;
        bool isAttackingOrSpell() const override { return false; }

        /// Since this action has no animation, apply a small cool down for using it
        float getActionCooldown() override { return 0.75f; }
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
        void prepare(const MWWorld::Ptr& actor) override;
        float getCombatRange (bool& isRanged) const override;
        const ESM::Weapon* getWeapon() const override;
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
