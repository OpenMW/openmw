#ifndef OPENMW_COMPONENTS_ESM_SERIALIZEREFID_HPP
#define OPENMW_COMPONENTS_ESM_SERIALIZEREFID_HPP

#include <charconv>
#include <cstring>
#include <limits>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>

namespace ESM
{
    constexpr std::string_view formIdRefIdPrefix = "FormId:";
    constexpr std::string_view generatedRefIdPrefix = "Generated:";
    constexpr std::string_view indexRefIdPrefix = "Index:";
    constexpr std::string_view esm3ExteriorCellRefIdPrefix = "Esm3ExteriorCell:";

    template <class T>
    std::size_t getDecIntegralCapacity(T value)
    {
        if (value == 0)
            return 1;
        constexpr std::size_t lastDigit = 1;
        if (value > 0)
            return static_cast<std::size_t>(std::numeric_limits<T>::digits10) + lastDigit;
        constexpr std::size_t sign = 1;
        return static_cast<std::size_t>(std::numeric_limits<T>::digits10) + lastDigit + sign;
    }

    template <class T>
    std::size_t getHexIntegralSize(T value)
    {
        static_assert(!std::is_signed_v<T>);
        std::size_t result = sizeof(T) * 2;
        while (true)
        {
            if (result == 1 || (value & static_cast<T>(0xf) << ((result - 1) * 4)) != 0)
                break;
            --result;
        }
        return result;
    }

    template <class T>
    std::size_t getHexIntegralSizeWith0x(T value)
    {
        return 2 + getHexIntegralSize(value);
    }

    inline void serializeRefIdPrefix(std::size_t valueSize, std::string_view prefix, std::string& out)
    {
        out.resize(prefix.size() + valueSize, '\0');
        std::memcpy(out.data(), prefix.data(), prefix.size());
    }

    template <class T>
    std::size_t serializeDecIntegral(T value, std::size_t shift, std::string& out)
    {
        const auto r = std::to_chars(out.data() + shift, out.data() + out.size(), value, 10);
        if (r.ec != std::errc())
            throw std::system_error(std::make_error_code(r.ec), "Failed to serialize ESM::RefId dec integral value");
        return r.ptr - out.data();
    }

    template <class T>
    void serializeHexIntegral(T value, std::size_t shift, std::string& out)
    {
        static_assert(!std::is_signed_v<T>);
        out[shift] = '0';
        out[shift + 1] = 'x';
        const auto r = std::to_chars(out.data() + shift + 2, out.data() + out.size(), value, 16);
        if (r.ec != std::errc())
            throw std::system_error(std::make_error_code(r.ec), "Failed to serialize ESM::RefId hex integral value");
    }

    template <class T>
    void serializeRefIdValue(T value, std::string_view prefix, std::string& out)
    {
        static_assert(!std::is_signed_v<T>);
        serializeRefIdPrefix(getHexIntegralSizeWith0x(value), prefix, out);
        serializeHexIntegral(value, prefix.size(), out);
    }

    template <class T>
    T deserializeDecIntegral(std::size_t shift, std::size_t end, std::string_view value)
    {
        T result{};
        const auto r = std::from_chars(value.data() + shift, value.data() + end, result, 10);
        if (r.ec != std::errc())
            throw std::system_error(std::make_error_code(r.ec),
                "Failed to deserialize ESM::RefId dec integral value: \""
                    + std::string(value.data() + shift, value.data() + end) + '"');
        return result;
    }

    template <class T>
    T deserializeHexIntegral(std::size_t shift, std::string_view value)
    {
        static_assert(!std::is_signed_v<T>);
        T result{};
        const auto r = std::from_chars(value.data() + shift + 2, value.data() + value.size(), result, 16);
        if (r.ec != std::errc())
            throw std::system_error(std::make_error_code(r.ec),
                "Failed to deserialize ESM::RefId hex integral value: \""
                    + std::string(value.data() + shift + 2, value.data() + value.size()) + '"');
        return result;
    }
}

#endif
