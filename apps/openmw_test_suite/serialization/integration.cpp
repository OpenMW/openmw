#include "format.hpp"

#include <components/serialization/binaryreader.hpp>
#include <components/serialization/binarywriter.hpp>
#include <components/serialization/sizeaccumulator.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

namespace
{
    using namespace testing;
    using namespace Serialization;
    using namespace SerializationTesting;

    struct DetourNavigatorSerializationIntegrationTest : Test
    {
        Composite mComposite;

        DetourNavigatorSerializationIntegrationTest()
        {
            mComposite.mIntVector = { 4, 5, 6 };
            mComposite.mEnumVector = { Enum::A, Enum::B, Enum::C };
            mComposite.mPodVector = { Pod{ 4, 23.87 }, Pod{ 5, -31.76 }, Pod{ 6, 65.12 } };
            mComposite.mPodBuffer = { Pod{ 7, 456.123 }, Pod{ 8, -628.346 } };
            mComposite.mPodDataSize = mComposite.mPodBuffer.size();
            std::string charData = "serialization";
            mComposite.mCharBuffer = { charData.begin(), charData.end() };
            mComposite.mCharDataSize = charData.size();
        }
    };

    TEST_F(DetourNavigatorSerializationIntegrationTest, sizeAccumulatorShouldSupportCustomSerializer)
    {
        SizeAccumulator sizeAccumulator;
        TestFormat<Mode::Write>{}(sizeAccumulator, mComposite);
        EXPECT_EQ(sizeAccumulator.value(), 143);
    }

    TEST_F(DetourNavigatorSerializationIntegrationTest, binaryReaderShouldDeserializeDataWrittenByBinaryWriter)
    {
        std::vector<std::byte> data(143);
        TestFormat<Mode::Write>{}(BinaryWriter(data.data(), data.data() + data.size()), mComposite);
        Composite result;
        TestFormat<Mode::Read>{}(BinaryReader(data.data(), data.data() + data.size()), result);
        EXPECT_EQ(result.mIntVector, mComposite.mIntVector);
        EXPECT_EQ(result.mEnumVector, mComposite.mEnumVector);
        EXPECT_EQ(result.mPodVector, mComposite.mPodVector);
        EXPECT_EQ(result.mPodDataSize, mComposite.mPodDataSize);
        EXPECT_EQ(result.mPodBuffer, mComposite.mPodBuffer);
        EXPECT_EQ(result.mCharDataSize, mComposite.mCharDataSize);
        EXPECT_EQ(result.mCharBuffer, mComposite.mCharBuffer);
    }
}
