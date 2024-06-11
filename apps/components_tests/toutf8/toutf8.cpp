#include <components/misc/strings/conversion.hpp>
#include <components/to_utf8/to_utf8.hpp>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#ifndef OPENMW_PROJECT_SOURCE_DIR
#define OPENMW_PROJECT_SOURCE_DIR "."
#endif

namespace
{
    using namespace testing;
    using namespace ToUTF8;

    struct Params
    {
        FromType mLegacyEncoding;
        std::string mLegacyEncodingFileName;
        std::string mUtf8FileName;
    };

    std::string readContent(const std::string& fileName)
    {
        std::ifstream file;
        file.exceptions(std::ios::failbit | std::ios::badbit);
        file.open(std::filesystem::path{ OPENMW_PROJECT_SOURCE_DIR } / "apps" / "components_tests" / "toutf8" / "data"
            / Misc::StringUtils::stringToU8String(fileName));
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    struct Utf8EncoderTest : TestWithParam<Params>
    {
    };

    TEST(Utf8EncoderTest, getUtf8ShouldReturnEmptyAsIs)
    {
        Utf8Encoder encoder(FromType::CP437);
        EXPECT_EQ(encoder.getUtf8(std::string_view()), std::string_view());
    }

    TEST(Utf8EncoderTest, getUtf8ShouldReturnAsciiOnlyAsIs)
    {
        std::string input;
        for (int c = 1; c <= std::numeric_limits<char>::max(); ++c)
            input.push_back(static_cast<char>(c));
        Utf8Encoder encoder(FromType::CP437);
        const std::string_view result = encoder.getUtf8(input);
        EXPECT_EQ(result.data(), input.data());
        EXPECT_EQ(result.size(), input.size());
    }

    TEST(Utf8EncoderTest, getUtf8ShouldLookUpUntilZero)
    {
        const std::string input("a\0b");
        Utf8Encoder encoder(FromType::CP437);
        const std::string_view result = encoder.getUtf8(input);
        EXPECT_EQ(result, "a");
    }

    TEST(Utf8EncoderTest, getUtf8ShouldLookUpUntilEndOfInputForAscii)
    {
        const std::string input("abc");
        Utf8Encoder encoder(FromType::CP437);
        const std::string_view result = encoder.getUtf8(std::string_view(input.data(), 2));
        EXPECT_EQ(result, "ab");
    }

    TEST(Utf8EncoderTest, getUtf8ShouldLookUpUntilEndOfInputForNonAscii)
    {
        const std::string input(
            "a\x92"
            "b");
        Utf8Encoder encoder(FromType::WINDOWS_1252);
        const std::string_view result = encoder.getUtf8(std::string_view(input.data(), 2));
        EXPECT_EQ(result, "a\xE2\x80\x99");
    }

    TEST_P(Utf8EncoderTest, getUtf8ShouldConvertFromLegacyEncodingToUtf8)
    {
        const std::string input(readContent(GetParam().mLegacyEncodingFileName));
        const std::string expected(readContent(GetParam().mUtf8FileName));
        Utf8Encoder encoder(GetParam().mLegacyEncoding);
        const std::string_view result = encoder.getUtf8(input);
        EXPECT_EQ(result, expected);
    }

    TEST(Utf8EncoderTest, getLegacyEncShouldReturnEmptyAsIs)
    {
        Utf8Encoder encoder(FromType::CP437);
        EXPECT_EQ(encoder.getLegacyEnc(std::string_view()), std::string_view());
    }

    TEST(Utf8EncoderTest, getLegacyEncShouldReturnAsciiOnlyAsIs)
    {
        std::string input;
        for (int c = 1; c <= std::numeric_limits<char>::max(); ++c)
            input.push_back(static_cast<char>(c));
        Utf8Encoder encoder(FromType::CP437);
        const std::string_view result = encoder.getLegacyEnc(input);
        EXPECT_EQ(result.data(), input.data());
        EXPECT_EQ(result.size(), input.size());
    }

    TEST(Utf8EncoderTest, getLegacyEncShouldLookUpUntilZero)
    {
        const std::string input("a\0b");
        Utf8Encoder encoder(FromType::CP437);
        const std::string_view result = encoder.getLegacyEnc(input);
        EXPECT_EQ(result, "a");
    }

    TEST(Utf8EncoderTest, getLegacyEncShouldLookUpUntilEndOfInputForAscii)
    {
        const std::string input("abc");
        Utf8Encoder encoder(FromType::CP437);
        const std::string_view result = encoder.getLegacyEnc(std::string_view(input.data(), 2));
        EXPECT_EQ(result, "ab");
    }

    TEST(Utf8EncoderTest, getLegacyEncShouldStripIncompleteCharacters)
    {
        const std::string input("a\xc3\xa2\xe2\x80\x99");
        Utf8Encoder encoder(FromType::WINDOWS_1252);
        const std::string_view result = encoder.getLegacyEnc(std::string_view(input.data(), 5));
        EXPECT_EQ(result, "a\xe2");
    }

    TEST_P(Utf8EncoderTest, getLegacyEncShouldConvertFromUtf8ToLegacyEncoding)
    {
        const std::string input(readContent(GetParam().mUtf8FileName));
        const std::string expected(readContent(GetParam().mLegacyEncodingFileName));
        Utf8Encoder encoder(GetParam().mLegacyEncoding);
        const std::string_view result = encoder.getLegacyEnc(input);
        EXPECT_EQ(result, expected);
    }

    INSTANTIATE_TEST_SUITE_P(Files, Utf8EncoderTest,
        Values(Params{ ToUTF8::WINDOWS_1251, "russian-win1251.txt", "russian-utf8.txt" },
            Params{ ToUTF8::WINDOWS_1252, "french-win1252.txt", "french-utf8.txt" }));

    TEST(StatelessUtf8EncoderTest, shouldCleanupBuffer)
    {
        std::string buffer;
        StatelessUtf8Encoder encoder(FromType::WINDOWS_1252);
        encoder.getUtf8(std::string_view("long string\x92"), BufferAllocationPolicy::UseGrowFactor, buffer);
        const std::string shortString("short\x92");
        ASSERT_GT(buffer.size(), shortString.size());
        const std::string_view shortUtf8 = encoder.getUtf8(shortString, BufferAllocationPolicy::UseGrowFactor, buffer);
        ASSERT_GE(buffer.size(), shortUtf8.size());
        EXPECT_EQ(buffer[shortUtf8.size()], '\0') << buffer;
    }

    TEST(StatelessUtf8EncoderTest, withFitToRequiredSizeShouldResizeBuffer)
    {
        std::string buffer;
        StatelessUtf8Encoder encoder(FromType::WINDOWS_1252);
        const std::string_view utf8
            = encoder.getUtf8(std::string_view("long string\x92"), BufferAllocationPolicy::FitToRequiredSize, buffer);
        EXPECT_EQ(buffer.size(), utf8.size());
    }
}
