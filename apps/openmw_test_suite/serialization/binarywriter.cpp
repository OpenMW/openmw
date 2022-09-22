#include "format.hpp"

#include <components/serialization/binarywriter.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <vector>

namespace
{
    using namespace testing;
    using namespace Serialization;
    using namespace SerializationTesting;

    TEST(DetourNavigatorSerializationBinaryWriterTest, shouldWriteArithmeticTypeValue)
    {
        std::vector<std::byte> result(4);
        BinaryWriter binaryWriter(result.data(), result.data() + result.size());
        const TestFormat<Mode::Write> format;
        binaryWriter(format, std::uint32_t(42));
        EXPECT_THAT(result, ElementsAre(std::byte(42), std::byte(0), std::byte(0), std::byte(0)));
    }

    TEST(DetourNavigatorSerializationBinaryWriterTest, shouldWriteArithmeticTypeRangeValue)
    {
        std::vector<std::byte> result(8);
        BinaryWriter binaryWriter(result.data(), result.data() + result.size());
        std::vector<std::uint32_t> values({ 42, 13 });
        const TestFormat<Mode::Write> format;
        binaryWriter(format, values.data(), values.size());
        constexpr std::array<std::byte, 8> expected{
            std::byte(42),
            std::byte(0),
            std::byte(0),
            std::byte(0),
            std::byte(13),
            std::byte(0),
            std::byte(0),
            std::byte(0),
        };
        EXPECT_THAT(result, ElementsAreArray(expected));
    }

    TEST(DetourNavigatorSerializationBinaryWriterTest, forNotEnoughSpaceForArithmeticTypeShouldThrowException)
    {
        std::vector<std::byte> result(3);
        BinaryWriter binaryWriter(result.data(), result.data() + result.size());
        const TestFormat<Mode::Write> format;
        EXPECT_THROW(binaryWriter(format, std::uint32_t(42)), std::runtime_error);
    }

    TEST(DetourNavigatorSerializationBinaryWriterTest, forNotEnoughSpaceForArithmeticTypeRangeShouldThrowException)
    {
        std::vector<std::byte> result(7);
        BinaryWriter binaryWriter(result.data(), result.data() + result.size());
        std::vector<std::uint32_t> values({ 42, 13 });
        const TestFormat<Mode::Write> format;
        EXPECT_THROW(binaryWriter(format, values.data(), values.size()), std::runtime_error);
    }
}
