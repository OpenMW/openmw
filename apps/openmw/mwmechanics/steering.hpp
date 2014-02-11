#ifndef OPENMW_MECHANICS_STEERING_H

#include <OgreMath.h>

namespace MWWorld
{
class Ptr;
}

namespace MWMechanics
{

/// configure rotation settings for an actor to reach this target angle (eventually)
/// @return have we reached the target angle?
bool zTurn(const MWWorld::Ptr& actor, Ogre::Radian targetAngle);

}

#endif
