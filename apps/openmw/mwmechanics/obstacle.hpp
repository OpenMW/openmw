#ifndef OPENMW_MECHANICS_OBSTACLE_H
#define OPENMW_MECHANICS_OBSTACLE_H

namespace MWWorld
{
    class Ptr;
}

namespace MWMechanics
{
    /// NOTE: determined empirically based on in-game behaviour
    static const float MIN_DIST_TO_DOOR_SQUARED = 128*128;

    /// tests actor's proximity to a closed door by default
    bool proximityToDoor(const MWWorld::Ptr& actor,
                         float minSqr = MIN_DIST_TO_DOOR_SQUARED,
                         bool closed = true);

    /// Returns door pointer within range. No guarentee is given as too which one
    /** \return Pointer to the door, or NULL if none exists **/
    MWWorld::Ptr getNearbyDoor(const MWWorld::Ptr& actor,
                         float minSqr = MIN_DIST_TO_DOOR_SQUARED,
                         bool closed = true);

    class ObstacleCheck
    {
        public:
            ObstacleCheck();

            // Clear the timers and set the state machine to default
            void clear();

            bool isNormalState() const;

            // Returns true if there is an obstacle and an evasive action
            // should be taken
            bool check(const MWWorld::Ptr& actor, float duration);

        private:

            // for checking if we're stuck (ignoring Z axis)
            float mPrevX;
            float mPrevY;

            enum WalkState
            {
                State_Norm,
                State_CheckStuck,
                State_Evade
            };
            WalkState mWalkState;

            float mStuckDuration; // accumulate time here while in same spot
            float mEvadeDuration;
            float mDistSameSpot; // take account of actor's speed
    };
}

#endif
