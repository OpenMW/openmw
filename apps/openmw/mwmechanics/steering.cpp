#include "steering.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/ptr.hpp"

#include "../mwbase/environment.hpp"

#include "movement.hpp"

namespace MWMechanics
{

bool smoothTurn(const MWWorld::Ptr& actor, float targetAngleRadians, int axis, float epsilonRadians)
{
    float currentAngle (actor.getRefData().getPosition().rot[axis]);
    float diff (targetAngleRadians - currentAngle);
    if (std::abs(diff) >= osg::DegreesToRadians(180.f))
    {
        if (diff >= 0)
        {
            diff = diff - osg::DegreesToRadians(360.f);
        }
        else
        {
            diff = osg::DegreesToRadians(360.f) + diff;
        }
    }
    float absDiff = std::abs(diff);

    // The turning animation actually moves you slightly, so the angle will be wrong again.
    // Use epsilon to prevent jerkiness.
    if (absDiff < epsilonRadians)
        return true;

    float limit = getAngularVelocity(actor.getClass().getSpeed(actor)) * MWBase::Environment::get().getFrameDuration();
    if (absDiff > limit)
        diff = osg::sign(diff) * limit;

    actor.getClass().getMovementSettings(actor).mRotation[axis] = diff;
    return false;
}

bool zTurn(const MWWorld::Ptr& actor, float targetAngleRadians, float epsilonRadians)
{
    return smoothTurn(actor, targetAngleRadians, 2, epsilonRadians);
}

}
