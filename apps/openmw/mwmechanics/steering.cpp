#include "steering.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/ptr.hpp"

#include "../mwbase/environment.hpp"

#include "movement.hpp"

namespace MWMechanics
{

bool smoothTurn(const MWWorld::Ptr& actor, Ogre::Radian targetAngle, int axis, Ogre::Degree epsilon)
{
    Ogre::Radian currentAngle (actor.getRefData().getPosition().rot[axis]);
    Ogre::Radian diff (targetAngle - currentAngle);
    if (diff >= Ogre::Degree(180))
    {
        // Turning the other way would be a better idea
        diff = diff-Ogre::Degree(360);
    }
    else if (diff <= Ogre::Degree(-180))
    {
        diff = Ogre::Degree(360)-diff;
    }
    Ogre::Radian absDiff = Ogre::Math::Abs(diff);

    // The turning animation actually moves you slightly, so the angle will be wrong again.
    // Use epsilon to prevent jerkiness.
    if (absDiff < epsilon)
        return true;

    Ogre::Radian limit = MAX_VEL_ANGULAR * MWBase::Environment::get().getFrameDuration();
    if (absDiff > limit)
        diff = Ogre::Math::Sign(diff) * limit;

    actor.getClass().getMovementSettings(actor).mRotation[axis] = diff.valueRadians();
    return false;
}

bool zTurn(const MWWorld::Ptr& actor, Ogre::Radian targetAngle, Ogre::Degree epsilon)
{
    return smoothTurn(actor, targetAngle, 2, epsilon);
}

}
