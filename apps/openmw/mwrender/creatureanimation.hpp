#ifndef GAME_RENDER_CREATUREANIMATION_H
#define GAME_RENDER_CREATUREANIMATION_H

#include "animation.hpp"
#include "weaponanimation.hpp"
#include "../mwworld/inventorystore.hpp"

namespace MWWorld
{
    class Ptr;
}

namespace MWRender
{
    class CreatureAnimation : public Animation
    {
    public:
        CreatureAnimation(const MWWorld::Ptr& ptr, const std::string &model);
        virtual ~CreatureAnimation() {}
    };

    // For creatures with weapons and shields
    // Animation is already virtual anyway, so might as well make a separate class.
    // Most creatures don't need weapons/shields, so this will save some memory.
    class CreatureWeaponAnimation : public Animation, public WeaponAnimation, public MWWorld::InventoryStoreListener
    {
    public:
        CreatureWeaponAnimation(const MWWorld::Ptr& ptr, const std::string &model);
        virtual ~CreatureWeaponAnimation() {}

        virtual void equipmentChanged() { updateParts(); }

        virtual void showWeapons(bool showWeapon);
        virtual void showCarriedLeft(bool show);

        void updateParts();

        void updatePart(NifOgre::ObjectScenePtr& scene, int slot);

        virtual void attachArrow();
        virtual void releaseArrow();

        virtual Ogre::Vector3 runAnimation(float duration);

        /// A relative factor (0-1) that decides if and how much the skeleton should be pitched
        /// to indicate the facing orientation of the character.
        virtual void setPitchFactor(float factor) { mPitchFactor = factor; }

        virtual void setWeaponGroup(const std::string& group) { mWeaponAnimationTime->setGroup(group); }

        // WeaponAnimation
        virtual NifOgre::ObjectScenePtr getWeapon() { return mWeapon; }
        virtual void showWeapon(bool show) { showWeapons(show); }
        virtual void configureAddedObject(NifOgre::ObjectScenePtr object, MWWorld::Ptr ptr, int slot);

    private:
        NifOgre::ObjectScenePtr mWeapon;
        NifOgre::ObjectScenePtr mShield;
        bool mShowWeapons;
        bool mShowCarriedLeft;

        Ogre::SharedPtr<WeaponAnimationTime> mWeaponAnimationTime;
    };
}

#endif
