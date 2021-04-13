#ifndef MISC_HASH_H
#define MISC_HASH_H

namespace Misc
{
    /// Implemented similar to the boost::hash_combine
    template <class T>
    inline void hashCombine(std::size_t& seed, const T& v)
    {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
    }
}

#endif