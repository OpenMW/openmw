#ifndef COMPONENTS_MISC_ENDIANNESS_H
#define COMPONENTS_MISC_ENDIANNESS_H

#include <cstdint>
#include <cstring>
#include <type_traits>

namespace Misc
{

    // Two-way conversion little-endian <-> big-endian
    template <typename T>
    void swapEndiannessInplace(T& v)
    {
        static_assert(std::is_arithmetic_v<T>);
        static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);

        if constexpr (sizeof(T) == 2)
        {
            uint16_t v16;
            std::memcpy(&v16, &v, sizeof(T));
            v16 = (v16 >> 8) | (v16 << 8);
            std::memcpy(&v, &v16, sizeof(T));
        }
        if constexpr (sizeof(T) == 4)
        {
            uint32_t v32;
            std::memcpy(&v32, &v, sizeof(T));
            v32 = (v32 >> 24) | ((v32 >> 8) & 0xff00) | ((v32 & 0xff00) << 8) | (v32 << 24);
            std::memcpy(&v, &v32, sizeof(T));
        }
        if constexpr (sizeof(T) == 8)
        {
            uint64_t v64;
            std::memcpy(&v64, &v, sizeof(T));
            v64 = (v64 >> 56) | ((v64 & 0x00ff'0000'0000'0000) >> 40) | ((v64 & 0x0000'ff00'0000'0000) >> 24)
                | ((v64 & 0x0000'00ff'0000'0000) >> 8) | ((v64 & 0x0000'0000'ff00'0000) << 8)
                | ((v64 & 0x0000'0000'00ff'0000) << 24) | ((v64 & 0x0000'0000'0000'ff00) << 40) | (v64 << 56);
            std::memcpy(&v, &v64, sizeof(T));
        }
    }

    #ifdef _WIN32
    constexpr bool IS_LITTLE_ENDIAN = true;
    constexpr bool IS_BIG_ENDIAN = false;
    #else
    constexpr bool IS_LITTLE_ENDIAN = __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__;
    constexpr bool IS_BIG_ENDIAN = __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__;
    #endif

    // Usage: swapEndiannessInplaceIf<IS_BIG_ENDIAN>(v)  - native to little-endian or back
    //        swapEndiannessInplaceIf<IS_LITTLE_ENDIAN>(v)  - native to big-endian or back
    template <bool C, typename T>
    void swapEndiannessInplaceIf(T& v)
    {
        static_assert(std::is_arithmetic_v<T>);
        static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);
        if constexpr (C)
            swapEndiannessInplace(v);
    }

    template <typename T>
    T toLittleEndian(T v)
    {
        swapEndiannessInplaceIf<IS_BIG_ENDIAN>(v);
        return v;
    }
    template <typename T>
    T fromLittleEndian(T v)
    {
        swapEndiannessInplaceIf<IS_BIG_ENDIAN>(v);
        return v;
    }

    template <typename T>
    T toBigEndian(T v)
    {
        swapEndiannessInplaceIf<IS_LITTLE_ENDIAN>(v);
        return v;
    }
    template <typename T>
    T fromBigEndian(T v)
    {
        swapEndiannessInplaceIf<IS_LITTLE_ENDIAN>(v);
        return v;
    }

}

#endif // COMPONENTS_MISC_ENDIANNESS_H
