#ifndef GAME_RENDER_CREATUREANIMATION_H
#define GAME_RENDER_CREATUREANIMATION_H

#include "animation.hpp"
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
        CreatureAnimation(const MWWorld::Ptr& ptr);
        virtual ~CreatureAnimation() {}
    };

    // For creatures with weapons and shields
    // Animation is already virtual anyway, so might as well make a separate class.
    // Most creatures don't need weapons/shields, so this will save some memory.
    class CreatureWeaponAnimation : public Animation, public MWWorld::InventoryStoreListener
    {
    public:
        CreatureWeaponAnimation(const MWWorld::Ptr& ptr);
        virtual ~CreatureWeaponAnimation() {}

        virtual void equipmentChanged() { updateParts(); }

        virtual void showWeapons(bool showWeapon);
        virtual void showCarriedLeft(bool show);

        void updateParts();

        void updatePart(NifOgre::ObjectScenePtr& scene, int slot);

    private:
        NifOgre::ObjectScenePtr mWeapon;
        NifOgre::ObjectScenePtr mShield;
        bool mShowWeapons;
        bool mShowCarriedLeft;
    };
}

#endif
