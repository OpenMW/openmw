#ifndef OPENMW_MECHANICS_OBSTACLE_H
#define OPENMW_MECHANICS_OBSTACLE_H

namespace MWWorld
{
    class Ptr;
}

namespace MWMechanics
{
    struct Movement;

    static const int NUM_EVADE_DIRECTIONS = 4;

    /// tests actor's proximity to a closed door by default
    bool proximityToDoor(const MWWorld::Ptr& actor, float minDist);

    /// Returns door pointer within range. No guarantee is given as to which one
    /** \return Pointer to the door, or empty pointer if none exists **/
    const MWWorld::Ptr getNearbyDoor(const MWWorld::Ptr& actor, float minDist);

    class ObstacleCheck
    {
        public:
            ObstacleCheck();

            // Clear the timers and set the state machine to default
            void clear();

            bool isEvading() const;

            // Updates internal state, call each frame for moving actor
            void update(const MWWorld::Ptr& actor, float duration);

            // change direction to try to fix "stuck" actor
            void takeEvasiveAction(MWMechanics::Movement& actorMovement) const;

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
