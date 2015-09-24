#ifndef OPENMW_MECHANICS_STEERING_H
#define OPENMW_MECHANICS_STEERING_H

#include <osg/Math>

namespace MWWorld
{
class Ptr;
}

namespace MWMechanics
{

// Max rotating speed, radian/sec
const float MAX_VEL_ANGULAR_RADIANS(10);

/// configure rotation settings for an actor to reach this target angle (eventually)
/// @return have we reached the target angle?
bool zTurn(const MWWorld::Ptr& actor, float targetAngleRadians,
                                      float epsilonRadians = osg::DegreesToRadians(0.5));

bool smoothTurn(const MWWorld::Ptr& actor, float targetAngleRadians, int axis,
                                      float epsilonRadians = osg::DegreesToRadians(0.5));

}

#endif
