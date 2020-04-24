#ifndef OPENMW_MECHANICS_STEERING_H
#define OPENMW_MECHANICS_STEERING_H

#include <osg/Math>

#include <algorithm>

namespace MWWorld
{
class Ptr;
}

namespace MWMechanics
{

// Max rotating speed, radian/sec
inline float getAngularVelocity(const float actorSpeed)
{
    const float baseAngluarVelocity = 10;
    const float baseSpeed = 200;
    return baseAngluarVelocity * std::max(actorSpeed / baseSpeed, 1.0f);
}

/// configure rotation settings for an actor to reach this target angle (eventually)
/// @return have we reached the target angle?
bool zTurn(const MWWorld::Ptr& actor, float targetAngleRadians,
                                      float epsilonRadians = osg::DegreesToRadians(0.5));

bool smoothTurn(const MWWorld::Ptr& actor, float targetAngleRadians, int axis,
                                      float epsilonRadians = osg::DegreesToRadians(0.5));

}

#endif
