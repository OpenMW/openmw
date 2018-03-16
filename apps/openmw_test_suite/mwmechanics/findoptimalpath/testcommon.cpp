#include "apps/openmw/mwmechanics/findoptimalpath/common.hpp"

#include <gtest/gtest.h>

namespace
{

    using namespace testing;
    using namespace MWMechanics;

    using MWMechanics::FindOptimalPath::isWalkableSlope;

    struct IsWalkableSlope : Test {};

    TEST(IsWalkableSlope, with_cos)
    {
        EXPECT_TRUE(isWalkableSlope(btScalar(1)));
        EXPECT_TRUE(isWalkableSlope(btScalar(0.5) + std::numeric_limits<btScalar>::epsilon()));
        EXPECT_TRUE(isWalkableSlope(btScalar(0.5)));
        EXPECT_FALSE(isWalkableSlope(btScalar(0.5) - std::numeric_limits<btScalar>::epsilon()));
        EXPECT_FALSE(isWalkableSlope(btScalar(0)));
    }

    TEST(IsWalkableSlope, with_vector)
    {
        EXPECT_TRUE(isWalkableSlope(btVector3(0, 0, 1)));
        EXPECT_FALSE(isWalkableSlope(btVector3(1, 0, 0)));
    }

}
