#ifndef OPENMW_MECHANICS_OBSTACLE_H
#define OPENMW_MECHANICS_OBSTACLE_H

#include "apps/openmw/mwworld/movementdirection.hpp"

#include <osg/Vec3f>

namespace MWWorld
{
    class Ptr;
    class ConstPtr;
}

namespace MWMechanics
{
    struct Movement;

    /// tests actor's proximity to a closed door by default
    bool proximityToDoor(const MWWorld::Ptr& actor, float minDist);

    /// Returns door pointer within range. No guarantee is given as to which one
    /** \return Pointer to the door, or empty pointer if none exists **/
    const MWWorld::Ptr getNearbyDoor(const MWWorld::Ptr& actor, float minDist);

    bool isAreaOccupiedByOtherActor(const MWWorld::ConstPtr& actor, const osg::Vec3f& destination);

    class ObstacleCheck
    {
    public:
        ObstacleCheck();

        // Clear the timers and set the state machine to default
        void clear();

        bool isEvading() const;

        // Updates internal state, call each frame for moving actor
        void update(const MWWorld::Ptr& actor, const osg::Vec3f& destination, float duration,
            MWWorld::MovementDirectionFlags supportedMovementDirection);

        // change direction to try to fix "stuck" actor
        void takeEvasiveAction(Movement& actorMovement) const;

    private:
        enum class WalkState
        {
            Initial,
            Norm,
            CheckStuck,
            Evade,
        };

        WalkState mWalkState = WalkState::Initial;
        float mStateDuration = 0;
        float mInitialDistance = 0;
        std::size_t mEvadeDirectionIndex;
        osg::Vec3f mPrev;
        osg::Vec3f mDestination;
    };
}

#endif
