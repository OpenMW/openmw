#ifndef GAME_MWMECHANICS_MOVEMENT_H
#define GAME_MWMECHANICS_MOVEMENT_H

namespace MWMechanics
{
    /// Desired movement for an actor
    struct Movement
    {
        signed char mLeftRight; // 1: wants to move left, -1: wants to move right
        signed char mForwardBackward; // 1:wants to move forward, -1: wants to move backward

        Movement() : mLeftRight (0), mForwardBackward (0) {}
    };
}

#endif
