#include "format.hpp"

#include <components/detournavigator/serialization/binaryreader.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>
#include <cstring>
#include <vector>

namespace
{
    using namespace testing;
    using namespace DetourNavigator::Serialization;
    using namespace DetourNavigator::SerializationTesting;

    TEST(DetourNavigatorSerializationBinaryReaderTest, shouldReadArithmeticTypeValue)
    {
        std::uint32_t value = 42;
        std::vector<std::byte> data(sizeof(value));
        std::memcpy(data.data(), &value, sizeof(value));
        BinaryReader binaryReader(data.data(), data.data() + data.size());
        std::uint32_t result = 0;
        const TestFormat<Mode::Read> format;
        binaryReader(format, result);
        EXPECT_EQ(result, 42);
    }

    TEST(DetourNavigatorSerializationBinaryReaderTest, shouldReadArithmeticTypeRangeValue)
    {
        const std::size_t count = 3;
        std::vector<std::byte> data(sizeof(std::size_t) + count * sizeof(std::uint32_t));
        std::memcpy(data.data(), &count, sizeof(count));
        const std::uint32_t value1 = 960900021;
        std::memcpy(data.data() + sizeof(count), &value1, sizeof(std::uint32_t));
        const std::uint32_t value2 = 1235496234;
        std::memcpy(data.data() + sizeof(count) + sizeof(std::uint32_t), &value2, sizeof(std::uint32_t));
        const std::uint32_t value3 = 2342038092;
        std::memcpy(data.data() + sizeof(count) + 2 * sizeof(std::uint32_t), &value3, sizeof(std::uint32_t));
        BinaryReader binaryReader(data.data(), data.data() + data.size());
        std::size_t resultCount = 0;
        const TestFormat<Mode::Read> format;
        binaryReader(format, resultCount);
        std::vector<std::uint32_t> result(resultCount);
        binaryReader(format, result.data(), result.size());
        EXPECT_THAT(result, ElementsAre(value1, value2, value3));
    }

    TEST(DetourNavigatorSerializationBinaryReaderTest, forNotEnoughDataForArithmeticTypeShouldThrowException)
    {
        std::vector<std::byte> data(3);
        BinaryReader binaryReader(data.data(), data.data() + data.size());
        std::uint32_t result = 0;
        const TestFormat<Mode::Read> format;
        EXPECT_THROW(binaryReader(format, result), std::runtime_error);
    }

    TEST(DetourNavigatorSerializationBinaryReaderTest, forNotEnoughDataForArithmeticTypeRangeShouldThrowException)
    {
        std::vector<std::byte> data(7);
        BinaryReader binaryReader(data.data(), data.data() + data.size());
        std::vector<std::uint32_t> values(2);
        const TestFormat<Mode::Read> format;
        EXPECT_THROW(binaryReader(format, values.data(), values.size()), std::runtime_error);
    }
}
