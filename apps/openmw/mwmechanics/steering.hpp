#ifndef OPENMW_MECHANICS_STEERING_H

#include <OgreMath.h>

namespace MWWorld
{
class Ptr;
}

namespace MWMechanics
{

// Max rotating speed, radian/sec
const Ogre::Radian MAX_VEL_ANGULAR(10);

/// configure rotation settings for an actor to reach this target angle (eventually)
/// @return have we reached the target angle?
bool zTurn(const MWWorld::Ptr& actor, Ogre::Radian targetAngle);

bool smoothTurn(const MWWorld::Ptr& actor, Ogre::Radian targetAngle, int axis);

}

#endif
