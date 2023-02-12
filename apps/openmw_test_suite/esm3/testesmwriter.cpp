#include <components/esm3/esmwriter.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <random>

namespace ESM
{
    namespace
    {
        using namespace ::testing;

        struct Esm3EsmWriterTest : public Test
        {
            std::minstd_rand mRandom;
            std::uniform_int_distribution<short> mRefIdDistribution{ 'a', 'z' };

            std::string generateRandomString(std::size_t size)
            {
                std::string result;
                std::generate_n(
                    std::back_inserter(result), size, [&] { return static_cast<char>(mRefIdDistribution(mRandom)); });
                return result;
            }
        };

        TEST_F(Esm3EsmWriterTest, saveShouldThrowExceptionOnWhenTruncatingHeaderStrings)
        {
            const std::string author = generateRandomString(33);
            const std::string description = generateRandomString(257);

            std::stringstream stream;

            ESMWriter writer;
            writer.setAuthor(author);
            writer.setDescription(description);
            writer.setFormatVersion(MaxLimitedSizeStringsFormatVersion);
            EXPECT_THROW(writer.save(stream), std::runtime_error);
        }

        TEST_F(Esm3EsmWriterTest, writeFixedStringShouldThrowExceptionOnTruncate)
        {
            std::stringstream stream;

            ESMWriter writer;
            writer.setFormatVersion(MaxLimitedSizeStringsFormatVersion);
            writer.save(stream);
            EXPECT_THROW(writer.writeMaybeFixedSizeString(generateRandomString(33), 32), std::runtime_error);
        }
    }
}
