#include "format.hpp"

#include <components/serialization/sizeaccumulator.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <variant>
#include <vector>

namespace
{
    using namespace testing;
    using namespace Serialization;
    using namespace SerializationTesting;

    TEST(DetourNavigatorSerializationSizeAccumulatorTest, shouldProvideSizeForArithmeticType)
    {
        SizeAccumulator sizeAccumulator;
        constexpr std::monostate format;
        sizeAccumulator(format, std::uint32_t());
        EXPECT_EQ(sizeAccumulator.value(), 4);
    }

    TEST(DetourNavigatorSerializationSizeAccumulatorTest, shouldProvideSizeForArithmeticTypeRange)
    {
        SizeAccumulator sizeAccumulator;
        const std::uint64_t* const data = nullptr;
        const std::size_t count = 3;
        const std::monostate format;
        sizeAccumulator(format, data, count);
        EXPECT_EQ(sizeAccumulator.value(), 24);
    }

    TEST(DetourNavigatorSerializationSizeAccumulatorTest, shouldSupportCustomSerializer)
    {
        SizeAccumulator sizeAccumulator;
        const TestFormat<Mode::Write> format;
        sizeAccumulator(format, Pod{});
        EXPECT_EQ(sizeAccumulator.value(), 12);
    }
}
