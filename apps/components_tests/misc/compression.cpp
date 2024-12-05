#include <components/misc/compression.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace
{
    using namespace testing;
    using namespace Misc;

    TEST(MiscCompressionTest, compressShouldAddPrefixWithDataSize)
    {
        const std::vector<std::byte> data(1234);
        const std::vector<std::byte> compressed = compress(data);
        int size = 0;
        std::memcpy(&size, compressed.data(), sizeof(size));
        EXPECT_EQ(size, data.size());
    }

    TEST(MiscCompressionTest, decompressIsInverseToCompress)
    {
        const std::vector<std::byte> data(1024);
        const std::vector<std::byte> compressed = compress(data);
        EXPECT_LT(compressed.size(), data.size());
        const std::vector<std::byte> decompressed = decompress(compressed);
        EXPECT_EQ(decompressed, data);
    }
}
