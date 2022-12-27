#include <gtest/gtest.h>

#include <cmath>

#include "apps/openmw/mwworld/duration.hpp"

namespace MWWorld
{
    namespace
    {
        TEST(MWWorldDurationTest, fromHoursShouldProduceZeroDaysAndHoursFor0)
        {
            const Duration duration = Duration::fromHours(0);
            EXPECT_EQ(duration.getDays(), 0);
            EXPECT_EQ(duration.getHours(), 0);
        }

        TEST(MWWorldDurationTest, fromHoursShouldProduceOneDayAndZeroHoursFor24)
        {
            const Duration duration = Duration::fromHours(24);
            EXPECT_EQ(duration.getDays(), 1);
            EXPECT_EQ(duration.getHours(), 0);
        }

        TEST(MWWorldDurationTest, fromHoursShouldProduceOneDayAndRemainderHoursFor42)
        {
            const Duration duration = Duration::fromHours(42);
            EXPECT_EQ(duration.getDays(), 1);
            EXPECT_EQ(duration.getHours(), 18);
        }

        TEST(MWWorldDurationTest, fromHoursShouldProduceZeroDaysAndZeroHoursForMinDouble)
        {
            const Duration duration = Duration::fromHours(std::numeric_limits<double>::min());
            EXPECT_EQ(duration.getDays(), 0);
            EXPECT_EQ(duration.getHours(), 0);
        }

        TEST(MWWorldDurationTest, fromHoursShouldProduceZeroDaysAndSomeHoursForMinFloat)
        {
            const Duration duration = Duration::fromHours(std::numeric_limits<float>::min());
            EXPECT_EQ(duration.getDays(), 0);
            EXPECT_GT(duration.getHours(), 0);
            EXPECT_FLOAT_EQ(duration.getHours(), std::numeric_limits<float>::min());
        }

        TEST(MWWorldDurationTest, fromHoursShouldProduceZeroDaysAndRemainderHoursForValueJustBelow24InDoublePrecision)
        {
            const Duration duration = Duration::fromHours(std::nextafter(24.0, 0.0));
            EXPECT_EQ(duration.getDays(), 0);
            EXPECT_LT(duration.getHours(), 24);
            EXPECT_FLOAT_EQ(duration.getHours(), 24);
        }

        TEST(MWWorldDurationTest, fromHoursShouldProduceZeroDaysAndRemainderHoursForValueJustBelow24InFloatPrecision)
        {
            const Duration duration = Duration::fromHours(std::nextafter(24.0f, 0.0f));
            EXPECT_EQ(duration.getDays(), 0);
            EXPECT_LT(duration.getHours(), 24);
            EXPECT_FLOAT_EQ(duration.getHours(), 24);
        }
    }
}
