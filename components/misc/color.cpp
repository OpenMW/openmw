#include "color.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <sstream>
#include <system_error>

namespace Misc
{
    Color::Color(float r, float g, float b, float a)
        : mR(std::clamp(r, 0.f, 1.f))
        , mG(std::clamp(g, 0.f, 1.f))
        , mB(std::clamp(b, 0.f, 1.f))
        , mA(std::clamp(a, 0.f, 1.f))
    {
    }

    std::string Color::toString() const
    {
        std::ostringstream ss;
        ss << "(" << r() << ", " << g() << ", " << b() << ", " << a() << ')';
        return ss.str();
    }

    Color Color::fromHex(std::string_view hex)
    {
        if (hex.size() != 6)
            throw std::logic_error(std::string("Invalid hex color: ") += hex);
        std::array<float, 3> rgb;
        for (size_t i = 0; i < rgb.size(); i++)
        {
            auto sub = hex.substr(i * 2, 2);
            int v = 0;
            auto [_, ec] = std::from_chars(sub.data(), sub.data() + sub.size(), v, 16);
            if (ec != std::errc())
                throw std::logic_error(std::string("Invalid hex color: ") += hex);
            rgb[i] = v / 255.0f;
        }
        return Color(rgb[0], rgb[1], rgb[2], 1);
    }

    std::string Color::toHex() const
    {
        std::string result(6, '0');
        std::array<float, 3> rgb = { mR, mG, mB };
        for (size_t i = 0; i < rgb.size(); i++)
        {
            int b = static_cast<int>(rgb[i] * 255.0f);
            char* start = result.data() + i * 2;
            if (b < 16)
                start++;
            auto [_, ec] = std::to_chars(start, result.data() + (i + 1) * 2, b, 16);
            if (ec != std::errc())
                throw std::logic_error("Error when converting number to base 16");
        }
        return result;
    }

    bool operator==(const Color& l, const Color& r)
    {
        return l.mR == r.mR && l.mG == r.mG && l.mB == r.mB && l.mA == r.mA;
    }
}
