#ifndef COMPONENTS_MISC_STRINGS_FORMAT_H
#define COMPONENTS_MISC_STRINGS_FORMAT_H

#include <cstdio>
#include <cerrno>
#include <cstring>
#include <string>
#include <string_view>
#include <stdexcept>

namespace Misc::StringUtils
{
    namespace Details
    {
        // Allow to convert complex arguments to C-style strings for format() function
        template <typename T>
        T argument(T value) noexcept
        {
            return value;
        }

        template <typename T>
        T const * argument(std::basic_string_view<T> const & value) noexcept
        {
            return value.data();
        }

        template <typename T>
        T const * argument(std::basic_string<T> const & value) noexcept
        {
            return value.c_str();
        }
    }

    // Requires some C++11 features:
    // 1. std::string needs to be contiguous
    // 2. std::snprintf with zero size (second argument) returns an output string size
    // 3. variadic templates support
    template <typename ... Args>
    std::string format(const char* fmt, Args const & ... args)
    {
        const int size = std::snprintf(nullptr, 0, fmt, Details::argument(args) ...);
        if (size < 0)
            throw std::runtime_error(std::string("Failed to compute resulting string size: ") + std::strerror(errno));
        // Note: sprintf also writes a trailing null character. We should remove it.
        std::string ret(static_cast<std::size_t>(size) + 1, '\0');
        if (std::sprintf(ret.data(), fmt, Details::argument(args) ...) < 0)
            throw std::runtime_error(std::string("Failed to format string: ") + std::strerror(errno));
        ret.erase(static_cast<std::size_t>(size));
        return ret;
    }

    template <typename ... Args>
    std::string format(const std::string& fmt, Args const & ... args)
    {
        return format(fmt.c_str(), args ...);
    }
}

#endif
