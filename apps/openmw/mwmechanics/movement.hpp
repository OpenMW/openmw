#ifndef GAME_MWMECHANICS_MOVEMENT_H
#define GAME_MWMECHANICS_MOVEMENT_H

namespace MWMechanics
{
    /// Desired movement for an actor
    struct Movement
    {
        float mPosition[3];
        float mRotation[3];

        Movement()
        {
            mPosition[0] = mPosition[1] = mPosition[2] = 0.0f;
            mRotation[0] = mRotation[1] = mRotation[2] = 0.0f;
        }
    };
}

#endif
