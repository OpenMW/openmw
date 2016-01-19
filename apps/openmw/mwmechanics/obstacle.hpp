#ifndef OPENMW_MECHANICS_OBSTACLE_H
#define OPENMW_MECHANICS_OBSTACLE_H

namespace MWWorld
{
    class Ptr;
}

namespace MWMechanics
{
    struct Movement;

    /// NOTE: determined empirically based on in-game behaviour
    static const float MIN_DIST_TO_DOOR_SQUARED = 128*128;

    static const int NUM_EVADE_DIRECTIONS = 4;

    /// tests actor's proximity to a closed door by default
    bool proximityToDoor(const MWWorld::Ptr& actor,
                         float minSqr = MIN_DIST_TO_DOOR_SQUARED);

    /// Returns door pointer within range. No guarentee is given as too which one
    /** \return Pointer to the door, or NULL if none exists **/
    MWWorld::Ptr getNearbyDoor(const MWWorld::Ptr& actor,
                         float minSqr = MIN_DIST_TO_DOOR_SQUARED);

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

            // change direction to try to fix "stuck" actor
            void takeEvasiveAction(MWMechanics::Movement& actorMovement);

        private:

            // for checking if we're stuck (ignoring Z axis)
            float mPrevX;
            float mPrevY;

            // directions to try moving in when get stuck
            static const float evadeDirections[NUM_EVADE_DIRECTIONS][2];

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
            int mEvadeDirectionIndex;

            void chooseEvasionDirection();
    };
}

#endif
