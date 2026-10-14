#ifndef OPENMW_MWMECHANICS_MOVINGTARGET_H
#define OPENMW_MWMECHANICS_MOVINGTARGET_H

#include <osg/Vec3f>

namespace MWMechanics
{
    /// Actor a magic bolt is aimed at, used to lead the shot if the actor is moving
    struct MovingTarget
    {
        osg::Vec3f mPosition; // Point to aim at
        osg::Vec3f mVelocity;
    };
}

#endif
