#ifndef OPENMW_COMPONENTS_ESM_SERIALIZEREFID_HPP
#define OPENMW_COMPONENTS_ESM_SERIALIZEREFID_HPP

#include <charconv>
#include <cstring>
#include <string>
#include <string_view>
#include <system_error>

namespace ESM
{
    constexpr std::string_view formIdRefIdPrefix = "FormId:";
    constexpr std::string_view generatedRefIdPrefix = "Generated:";
    constexpr std::string_view indexRefIdPrefix = "Index:";

    template <class T>
    std::size_t getIntegralSize(T value)
    {
        std::size_t result = sizeof(T) * 2;
        while (true)
        {
            if (result == 1 || (value & static_cast<T>(0xf) << ((result - 1) * 4)) != 0)
                break;
            --result;
        }
        return result;
    }

    inline void serializeRefIdPrefix(std::size_t valueSize, std::string_view prefix, std::string& out)
    {
        out.resize(prefix.size() + valueSize + 2, '\0');
        std::memcpy(out.data(), prefix.data(), prefix.size());
    }

    template <class T>
    void serializeIntegral(T value, std::size_t shift, std::string& out)
    {
        out[shift] = '0';
        out[shift + 1] = 'x';
        const auto r = std::to_chars(out.data() + shift + 2, out.data() + out.size(), value, 16);
        if (r.ec != std::errc())
            throw std::system_error(std::make_error_code(r.ec), "Failed to serialize ESM::RefId integral value");
    }

    template <class T>
    void serializeRefIdValue(T value, std::string_view prefix, std::string& out)
    {
        serializeRefIdPrefix(getIntegralSize(value), prefix, out);
        serializeIntegral(value, prefix.size(), out);
    }

    template <class T>
    T deserializeIntegral(std::size_t shift, std::string_view value)
    {
        T result{};
        const auto r = std::from_chars(value.data() + shift + 2, value.data() + value.size(), result, 16);
        if (r.ec != std::errc())
            throw std::system_error(std::make_error_code(r.ec),
                "Failed to deserialize ESM::RefId integral value: \"" + std::string(value) + '"');
        return result;
    }
}

#endif
