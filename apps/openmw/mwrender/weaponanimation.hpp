#ifndef OPENMW_MWRENDER_WEAPONANIMATION_H
#define OPENMW_MWRENDER_WEAPONANIMATION_H

#include <components/sceneutil/controller.hpp>

#include "../mwworld/ptr.hpp"
#include "animation.hpp"

namespace MWRender
{

    class WeaponAnimationTime : public SceneUtil::ControllerSource
    {
    private:
        Animation* mAnimation;
        std::string mWeaponGroup;
        float mStartTime;
    public:
        WeaponAnimationTime(Animation* animation) : mAnimation(animation), mStartTime(0) {}
        void setGroup(const std::string& group);
        void updateStartTime();

        virtual float getValue(osg::NodeVisitor* nv);
    };

    /// Handles attach & release of projectiles for ranged weapons
    class WeaponAnimation
    {
    public:
        WeaponAnimation();
        virtual ~WeaponAnimation() {}

        /// @note If no weapon (or an invalid weapon) is equipped, this function is a no-op.
        void attachArrow(MWWorld::Ptr actor);

        /// @note If no weapon (or an invalid weapon) is equipped, this function is a no-op.
        void releaseArrow(MWWorld::Ptr actor);

    protected:
        PartHolderPtr mAmmunition;

        virtual osg::Group* getArrowBone() = 0;
        virtual osg::Node* getWeaponNode() = 0;
        virtual Resource::ResourceSystem* getResourceSystem() = 0;


        //virtual NifOgre::ObjectScenePtr getWeapon() = 0;
        virtual void showWeapon(bool show) = 0;
        //virtual void configureAddedObject(NifOgre::ObjectScenePtr object, MWWorld::Ptr ptr, int slot) = 0;

        /// A relative factor (0-1) that decides if and how much the skeleton should be pitched
        /// to indicate the facing orientation of the character, for ranged weapon aiming.
        float mPitchFactor;

        //void pitchSkeleton(float xrot, Ogre::SkeletonInstance* skel);
    };

}

#endif
