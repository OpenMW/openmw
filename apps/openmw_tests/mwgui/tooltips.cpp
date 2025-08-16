#include <gtest/gtest.h>

#include "apps/openmw/mwgui/tooltips.hpp"

#include <limits>

namespace MWGui
{
    namespace
    {
        TEST(MWGuiToolTipsTest, floatsShouldBeFormattedCorrectly)
        {
            EXPECT_EQ(ToolTips::toString(1.f), "1");
            EXPECT_EQ(ToolTips::toString(1.1f), "1.1");
            EXPECT_EQ(ToolTips::toString(1.12f), "1.12");
            EXPECT_EQ(ToolTips::toString(1234567.12f), "1234567.12");
            EXPECT_EQ(ToolTips::toString(0.001f), "0");
            EXPECT_EQ(ToolTips::toString(0.01f), "0.01");
            EXPECT_EQ(ToolTips::toString(0.01f), "0.01");
            EXPECT_EQ(ToolTips::toString(std::numeric_limits<float>::infinity()), "inf");
            EXPECT_EQ(ToolTips::toString(std::numeric_limits<float>::quiet_NaN()), "nan");
        }
    }
}
