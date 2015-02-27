#ifndef OPENMW_MWRENDER_WEAPONANIMATION_H
#define OPENMW_MWRENDER_WEAPONANIMATION_H

#include <OgreController.h>

#include <components/nifogre/ogrenifloader.hpp>

#include "../mwworld/ptr.hpp"

namespace MWRender
{

    class Animation;

    class WeaponAnimationTime : public Ogre::ControllerValue<Ogre::Real>
    {
    private:
        Animation* mAnimation;
        std::string mWeaponGroup;
        float mStartTime;
    public:
        WeaponAnimationTime(Animation* animation) : mAnimation(animation), mStartTime(0) {}
        void setGroup(const std::string& group);
        void updateStartTime();

        virtual Ogre::Real getValue() const;
        virtual void setValue(Ogre::Real value)
        { }
    };

    /// Handles attach & release of projectiles for ranged weapons
    class WeaponAnimation
    {
    public:
        WeaponAnimation() : mPitchFactor(0) {}
        virtual ~WeaponAnimation() {}

        /// @note If no weapon (or an invalid weapon) is equipped, this function is a no-op.
        void attachArrow(MWWorld::Ptr actor);

        /// @note If no weapon (or an invalid weapon) is equipped, this function is a no-op.
        void releaseArrow(MWWorld::Ptr actor);

    protected:
        NifOgre::ObjectScenePtr mAmmunition;

        virtual NifOgre::ObjectScenePtr getWeapon() = 0;
        virtual void showWeapon(bool show) = 0;
        virtual void configureAddedObject(NifOgre::ObjectScenePtr object, MWWorld::Ptr ptr, int slot) = 0;

        /// A relative factor (0-1) that decides if and how much the skeleton should be pitched
        /// to indicate the facing orientation of the character, for ranged weapon aiming.
        float mPitchFactor;

        void pitchSkeleton(float xrot, Ogre::SkeletonInstance* skel);
    };

}

#endif
