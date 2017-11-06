#ifndef OPENMW_TEST_SUITE_MWMECHANICS_MATCHERS_HPP
#define OPENMW_TEST_SUITE_MWMECHANICS_MATCHERS_HPP

#include "apps/openmw/mwphysics/closestcollision.hpp"

#include <LinearMath/btVector3.h>

#include <gmock/gmock.h>

namespace
{

    using MWPhysics::getClosestCollision;

    MATCHER_P2(IsNearTo, other, notFarThan, "")
    {
        const auto distance = arg.distance(other);
        *result_listener << "is too far from " << other << ", distance is " << distance;
        return distance <= notFarThan;
    }

    MATCHER_P2(IsFarFrom, other, notCloseThan, "")
    {
        const auto distance = arg.distance(other);
        *result_listener << "is too close to " << other << ", distance is " << distance;
        return distance > notCloseThan;
    }

    MATCHER_P(IsOnGround, test, "")
    {
        const auto difference = test->getDistanceToGround(arg) - test->actor.halfExtents.z();
        *result_listener << "is lifted above ground by " << difference;
        return difference <= btScalar(2);
    }

    using Transition = std::pair<btVector3, btVector3>;

    MATCHER_P(MedianPointIsOnGround, test, "")
    {
        const auto median = btScalar(0.5) * (arg.first + arg.second);
        const auto difference = test->getDistanceToGround(median) - test->actor.halfExtents.z();
        *result_listener << "median " << median << " is lifted above ground by " << difference;
        return difference <= btScalar(2);
    }

    MATCHER_P(IsClear, test, "")
    {
        if (const auto collision = getClosestCollision(test->actor.object, arg.first, arg.second, test->collisionWorld)) {
            *result_listener << "transition is not clear, hit at " << collision->mEnd;
            return false;
        }
        return true;
    }

}

#endif // OPENMW_TEST_SUITE_MWMECHANICS_MATCHERS_HPP
