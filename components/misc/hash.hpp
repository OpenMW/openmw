#ifndef MISC_HASH_H
#define MISC_HASH_H

#include <cstddef>
#include <cstdint>
#include <functional>

namespace Misc
{
    /// Implemented similar to the boost::hash_combine
    template <class Seed, class T>
    inline void hashCombine(Seed& seed, const T& v)
    {
        static_assert(sizeof(Seed) >= sizeof(std::size_t), "Resulting hash will be truncated");
        std::hash<T> hasher;
        seed ^= static_cast<Seed>(hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2));
    }

    // Comes from https://stackoverflow.com/questions/2634690/good-hash-function-for-a-2d-index
    // Effective Java (2nd edition) is cited as the source
    inline std::size_t hash2dCoord(int32_t x, int32_t y)
    {
        return (53 + std::hash<int32_t>{}(x)) * 53 + std::hash<int32_t>{}(y);
    }
}

#endif
