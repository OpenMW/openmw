#ifndef OPENMW_MWRENDER_WEAPONANIMATION_H
#define OPENMW_MWRENDER_WEAPONANIMATION_H

#include <components/sceneutil/controller.hpp>

#include "../mwworld/ptr.hpp"
#include "animation.hpp"

namespace MWRender
{

    class RotateController;

    class WeaponAnimationTime : public SceneUtil::ControllerSource
    {
    private:
        Animation* mAnimation;
        std::string mWeaponGroup;
        float mStartTime;
        bool mRelativeTime;
    public:
        WeaponAnimationTime(Animation* animation) : mAnimation(animation), mStartTime(0), mRelativeTime(false) {}
        void setGroup(const std::string& group, bool relativeTime);
        void updateStartTime();

        virtual float getValue(osg::NodeVisitor* nv);
    };

    /// Handles attach & release of projectiles for ranged weapons
    class WeaponAnimation
    {
    public:
        WeaponAnimation();
        virtual ~WeaponAnimation();

        /// @note If no weapon (or an invalid weapon) is equipped, this function is a no-op.
        void attachArrow(MWWorld::Ptr actor);

        /// @note If no weapon (or an invalid weapon) is equipped, this function is a no-op.
        void releaseArrow(MWWorld::Ptr actor, float attackStrength);

        /// Add WeaponAnimation-related controllers to \a nodes and store the added controllers in \a map.
        void addControllers(const std::map<std::string, osg::ref_ptr<osg::MatrixTransform> >& nodes,
                std::multimap<osg::ref_ptr<osg::Node>, osg::ref_ptr<osg::NodeCallback> >& map, osg::Node* objectRoot);

        void deleteControllers();

        /// Configure controllers, should be called every animation frame.
        void configureControllers(float characterPitchRadians);

    protected:
        PartHolderPtr mAmmunition;

        osg::ref_ptr<RotateController> mSpineControllers[2];

        void setControllerRotate(const osg::Quat& rotate);
        void setControllerEnabled(bool enabled);

        virtual osg::Group* getArrowBone() = 0;
        virtual osg::Node* getWeaponNode() = 0;
        virtual Resource::ResourceSystem* getResourceSystem() = 0;

        virtual void showWeapon(bool show) = 0;

        /// A relative factor (0-1) that decides if and how much the skeleton should be pitched
        /// to indicate the facing orientation of the character, for ranged weapon aiming.
        float mPitchFactor;
    };

}

#endif
