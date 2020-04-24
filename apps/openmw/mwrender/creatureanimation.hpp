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

        virtual void equipmentChanged() { updateParts(); }

        virtual void showWeapons(bool showWeapon);

        virtual bool getCarriedLeftShown() const { return mShowCarriedLeft; }
        virtual void showCarriedLeft(bool show);

        void updateParts();

        void updatePart(PartHolderPtr& scene, int slot);

        virtual void attachArrow();
        virtual void releaseArrow(float attackStrength);
        // WeaponAnimation
        virtual osg::Group* getArrowBone();
        virtual osg::Node* getWeaponNode();
        virtual Resource::ResourceSystem* getResourceSystem();
        virtual void showWeapon(bool show) { showWeapons(show); }
        virtual void setWeaponGroup(const std::string& group, bool relativeDuration) { mWeaponAnimationTime->setGroup(group, relativeDuration); }

        virtual void addControllers();

        virtual osg::Vec3f runAnimation(float duration);

        /// A relative factor (0-1) that decides if and how much the skeleton should be pitched
        /// to indicate the facing orientation of the character.
        virtual void setPitchFactor(float factor) { mPitchFactor = factor; }

    protected:
        virtual bool isArrowAttached() const;

    private:
        PartHolderPtr mWeapon;
        PartHolderPtr mShield;
        bool mShowWeapons;
        bool mShowCarriedLeft;

        std::shared_ptr<WeaponAnimationTime> mWeaponAnimationTime;
    };
}

#endif
