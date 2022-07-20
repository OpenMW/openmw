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
        seed ^= static_cast<Seed>(hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2));
    }
}

#endif
