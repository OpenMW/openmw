#ifndef OPENMW_COMPONENTS_MISC_FLOAT16_HPP
#define OPENMW_COMPONENTS_MISC_FLOAT16_HPP

#include <bit>
#include <cstdint>

namespace Misc
{
    using float16_t = std::uint16_t;

    inline float halfToFloat(const float16_t value)
    {
        std::uint32_t bits = static_cast<std::uint32_t>(value & 0x8000) << 16;

        const std::uint32_t exp16 = (value & 0x7c00) >> 10;
        std::uint32_t frac16 = value & 0x3ff;
        if (exp16)
            bits |= (exp16 + 0x70) << 23;
        else if (frac16)
        {
            std::uint8_t offset = 0;
            do
            {
                ++offset;
                frac16 <<= 1;
            } while ((frac16 & 0x400) != 0x400);
            frac16 &= 0x3ff;
            bits |= (0x71 - offset) << 23;
        }
        bits |= frac16 << 13;

        return std::bit_cast<float>(bits);
    }
}

#endif
