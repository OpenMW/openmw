//
// Created by koncord on 04.03.17.
//

#ifndef OPENMW_UTILS_HPP
#define OPENMW_UTILS_HPP

#include <components/openmw-mp/Utils.hpp>
#include <components/openmw-mp/Log.hpp>
#include <cstddef>
#include <vector>

#if (!defined(DEBUG_PRINTF) && defined(DEBUG))
#define DEBUG_PRINTF(...) LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, __VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif


namespace Utils
{
    const std::vector<std::string> split(const std::string &str, int delimiter);

    template<size_t N>
    constexpr unsigned int hash(const char(&str)[N], size_t I = N)
    {
        return (I == 1 ? ((2166136261u ^ str[0]) * 16777619u) : ((hash(str, I - 1) ^ str[I - 1]) * 16777619u));
    }

    inline unsigned int hash(const char *str, std::size_t I)
    {
        return (I == 1 ? ((2166136261u ^ str[0]) * 16777619u) : ((hash(str, I - 1) ^ str[I - 1]) * 16777619u));
    }

    template<typename F, typename T, typename E = void>
    struct is_static_castable : std::false_type
    {
    };
    template<typename F, typename T>
    struct is_static_castable<F, T, typename std::conditional<true, void, decltype(static_cast<T>(std::declval<F>()))>::type>
            : std::true_type
    {
    };

    template<typename T, typename F>
    inline static typename std::enable_if<is_static_castable<F *, T *>::value, T *>::type static_or_dynamic_cast(
            F *from)
    { return static_cast<T *>(from); }

    template<typename T, typename F>
    inline static typename std::enable_if<!is_static_castable<F *, T *>::value, T *>::type static_or_dynamic_cast(
            F *from)
    { return dynamic_cast<T *>(from); }
}


#endif //OPENMW_UTILS_HPP
