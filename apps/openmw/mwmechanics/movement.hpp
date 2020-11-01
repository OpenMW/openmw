#ifndef GAME_MWMECHANICS_MOVEMENT_H
#define GAME_MWMECHANICS_MOVEMENT_H

#include <osg/Vec3f>

namespace MWMechanics
{
    /// Desired movement for an actor
    struct Movement
    {
        // Desired movement. Direction is relative to the current orientation.
        // Length of the vector controls desired speed. 0 - stay, 0.5 - half-speed, 1.0 - max speed.
        float mPosition[3];
        // Desired rotation delta (euler angles).
        float mRotation[3];

        // Controlled by CharacterController, should not be changed from other places.
        // These fields can not be private fields in CharacterController, because Actor::getCurrentSpeed uses it.
        float mSpeedFactor;
        bool mIsStrafing;

        Movement()
        {
            mPosition[0] = mPosition[1] = mPosition[2] = 0.0f;
            mRotation[0] = mRotation[1] = mRotation[2] = 0.0f;
            mSpeedFactor = 1.f;
            mIsStrafing = false;
        }

        osg::Vec3f asVec3()
        {
            return osg::Vec3f(mPosition[0], mPosition[1], mPosition[2]);
        }
    };
}

#endif
