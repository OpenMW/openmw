#include "compression.hpp"

#include <lz4.h>

#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace Misc
{
    std::vector<std::byte> compress(const std::vector<std::byte>& data)
    {
        const std::size_t originalSize = data.size();
        std::vector<std::byte> result(static_cast<std::size_t>(LZ4_compressBound(static_cast<int>(originalSize)) + sizeof(originalSize)));
        const int size = LZ4_compress_default(
            reinterpret_cast<const char*>(data.data()),
            reinterpret_cast<char*>(result.data()) + sizeof(originalSize),
            static_cast<int>(data.size()),
            static_cast<int>(result.size() - sizeof(originalSize))
        );
        if (size == 0)
            throw std::runtime_error("Failed to compress");
        std::memcpy(result.data(), &originalSize, sizeof(originalSize));
        result.resize(static_cast<std::size_t>(size) + sizeof(originalSize));
        return result;
    }

    std::vector<std::byte> decompress(const std::vector<std::byte>& data)
    {
        std::size_t originalSize;
        std::memcpy(&originalSize, data.data(), sizeof(originalSize));
        std::vector<std::byte> result(originalSize);
        const int size = LZ4_decompress_safe(
            reinterpret_cast<const char*>(data.data()) + sizeof(originalSize),
            reinterpret_cast<char*>(result.data()),
            static_cast<int>(data.size() - sizeof(originalSize)),
            static_cast<int>(result.size())
        );
        if (size < 0)
            throw std::runtime_error("Failed to decompress");
        if (originalSize != static_cast<std::size_t>(size))
            throw std::runtime_error("Size of decompressed data (" + std::to_string(size)
                                     + ") doesn't match stored (" + std::to_string(originalSize) + ")");
        return result;
    }
}
