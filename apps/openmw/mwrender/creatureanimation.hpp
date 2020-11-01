#ifndef GAME_RENDER_CREATUREANIMATION_H
#define GAME_RENDER_CREATUREANIMATION_H

#include "actoranimation.hpp"
#include "weaponanimation.hpp"
#include "../mwworld/inventorystore.hpp"

namespace MWWorld
{
    class Ptr;
}

namespace MWRender
{
    class CreatureAnimation : public ActorAnimation
    {
    public:
        CreatureAnimation(const MWWorld::Ptr &ptr, const std::string& model, Resource::ResourceSystem* resourceSystem);
        virtual ~CreatureAnimation() {}
    };

    // For creatures with weapons and shields
    // Animation is already virtual anyway, so might as well make a separate class.
    // Most creatures don't need weapons/shields, so this will save some memory.
    class CreatureWeaponAnimation : public ActorAnimation, public WeaponAnimation, public MWWorld::InventoryStoreListener
    {
    public:
        CreatureWeaponAnimation(const MWWorld::Ptr &ptr, const std::string& model, Resource::ResourceSystem* resourceSystem);
        virtual ~CreatureWeaponAnimation() {}

        void equipmentChanged() override { updateParts(); }

        void showWeapons(bool showWeapon) override;

        bool getCarriedLeftShown() const override { return mShowCarriedLeft; }
        void showCarriedLeft(bool show) override;

        void updateParts();

        void updatePart(PartHolderPtr& scene, int slot);

        void attachArrow() override;
        void detachArrow() override;
        void releaseArrow(float attackStrength) override;
        // WeaponAnimation
        osg::Group* getArrowBone() override;
        osg::Node* getWeaponNode() override;
        Resource::ResourceSystem* getResourceSystem() override;
        void showWeapon(bool show) override { showWeapons(show); }
        void setWeaponGroup(const std::string& group, bool relativeDuration) override { mWeaponAnimationTime->setGroup(group, relativeDuration); }

        void addControllers() override;

        osg::Vec3f runAnimation(float duration) override;

        /// A relative factor (0-1) that decides if and how much the skeleton should be pitched
        /// to indicate the facing orientation of the character.
        void setPitchFactor(float factor) override { mPitchFactor = factor; }

    protected:
        bool isArrowAttached() const override;

    private:
        PartHolderPtr mWeapon;
        PartHolderPtr mShield;
        bool mShowWeapons;
        bool mShowCarriedLeft;

        std::shared_ptr<WeaponAnimationTime> mWeaponAnimationTime;
    };
}

#endif
