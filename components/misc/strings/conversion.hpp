#ifndef COMPONENTS_MISC_STRINGS_CONVERSION_H
#define COMPONENTS_MISC_STRINGS_CONVERSION_H

#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <system_error>

#if !(defined(_MSC_VER) && (_MSC_VER >= 1924)) && !(defined(__GNUC__) && __GNUC__ >= 11) || defined(__clang__)         \
    || defined(__apple_build_version__)

#include <ios>
#include <locale>
#include <sstream>

#endif

namespace Misc::StringUtils
{
    inline const char* u8StringToString(const char8_t* str)
    {
        return reinterpret_cast<const char*>(str);
    }

    inline char* u8StringToString(char8_t* str)
    {
        return reinterpret_cast<char*>(str);
    }

    inline std::string u8StringToString(std::u8string_view str)
    {
        return { str.begin(), str.end() };
    }

    inline std::string u8StringToString(std::u8string&& str)
    {
        return { str.begin(), str.end() };
    }

    inline const char8_t* stringToU8String(const char* str)
    {
        return reinterpret_cast<const char8_t*>(
            str); // Undefined behavior if the contents of "char" aren't UTF8 or ASCII.
    }

    inline char8_t* stringToU8String(char* str)
    {
        return reinterpret_cast<char8_t*>(str); // Undefined behavior if the contents of "char" aren't UTF8 or ASCII.
    }

    inline std::u8string stringToU8String(std::string_view str)
    {
        return { str.begin(), str.end() }; // Undefined behavior if the contents of "char" aren't UTF8 or ASCII.
    }

    inline std::u8string stringToU8String(std::string&& str)
    {
        return { str.begin(), str.end() }; // Undefined behavior if the contents of "char" aren't UTF8 or ASCII.
    }

    template <typename T>
    inline std::optional<T> toNumeric(std::string_view s)
    {
        T result{};
        auto [ptr, ec]{ std::from_chars(s.data(), s.data() + s.size(), result) };

        if (ec == std::errc())
        {
            return result;
        }

        return std::nullopt;
    }

    template <typename T>
    inline T toNumeric(std::string_view s, T defaultValue)
    {
        if (auto numeric = toNumeric<T>(s))
        {
            return *numeric;
        }

        return defaultValue;
    }

    // support for std::from_chars as of 2023-02-27
    // - Visual Studio 2019 version 16.4 (1924)
    // - GCC 11
    // - Clang does not support floating points yet
    // - Apples Clang does not support floating points yet

#if !(defined(_MSC_VER) && (_MSC_VER >= 1924)) && !(defined(__GNUC__) && __GNUC__ >= 11) || defined(__clang__)         \
    || defined(__apple_build_version__)
    template <>
    inline std::optional<float> toNumeric<float>(std::string_view s)
    {
        if (!s.empty())
        {
            std::istringstream iss(s.data());
            iss.imbue(std::locale::classic());

            float value;

            if (iss >> value)
            {
                return value;
            }
        }

        return std::nullopt;
    }

    template <>
    inline std::optional<double> toNumeric<double>(std::string_view s)
    {
        if (!s.empty())
        {
            std::istringstream iss(s.data());
            iss.imbue(std::locale::classic());

            double value;

            if (iss >> value)
            {
                return value;
            }
        }

        return std::nullopt;
    }
#endif

    inline std::string toHex(std::string_view value)
    {
        std::string buffer(value.size() * 2, '0');
        char* out = buffer.data();
        for (const char v : value)
        {
            const std::ptrdiff_t space = static_cast<std::ptrdiff_t>(static_cast<std::uint8_t>(v) <= 0xf);
            const auto [ptr, ec] = std::to_chars(out + space, out + space + 2, static_cast<std::uint8_t>(v), 16);
            if (ec != std::errc())
                throw std::system_error(std::make_error_code(ec));
            out += 2;
        }
        return buffer;
    }
}

#endif // COMPONENTS_MISC_STRINGS_CONVERSION_H
