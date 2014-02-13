#include "steering.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/ptr.hpp"

#include "../mwbase/environment.hpp"

#include "movement.hpp"

namespace MWMechanics
{

bool zTurn(const MWWorld::Ptr& actor, Ogre::Radian targetAngle)
{
    Ogre::Radian currentAngle (actor.getRefData().getPosition().rot[2]);
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
    const Ogre::Degree epsilon (0.5);
    if (absDiff < epsilon)
        return true;

    // Max. speed of 10 radian per sec
    Ogre::Radian limit = Ogre::Radian(10) * MWBase::Environment::get().getFrameDuration();
    if (absDiff > limit)
        diff = Ogre::Math::Sign(diff) * limit;

    actor.getClass().getMovementSettings(actor).mRotation[2] = diff.valueRadians();
    return false;
}

}
