#ifndef COMPONENTS_MISC_STRINGS_FORMAT_H
#define COMPONENTS_MISC_STRINGS_FORMAT_H

#include <MyGUI_UString.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace Misc::StringUtils
{
    namespace Details
    {
        // Allow to convert complex arguments to C-style strings for format() function
        template <typename T>
        T argument(T value) noexcept
        {
            static_assert(!std::is_same_v<T, std::string_view>, "std::string_view is not supported");
            static_assert(!std::is_same_v<T, MyGUI::UString>, "MyGUI::UString is not supported");
            return value;
        }

        template <typename T>
        T const* argument(std::basic_string<T> const& value) noexcept
        {
            return value.c_str();
        }

        template <class T>
        T nullTerminated(T value) noexcept
        {
            return value;
        }

        template <class T>
        std::basic_string<T> nullTerminated(const std::basic_string_view<T>& value) noexcept
        {
            // Ensure string_view arguments are null-terminated by creating a string
            // TODO: Use a format function that doesn't require this workaround
            return std::string{ value };
        }

        // Requires some C++11 features:
        // 1. std::string needs to be contiguous
        // 2. std::snprintf with zero size (second argument) returns an output string size
        // 3. variadic templates support
        template <typename... Args>
        std::string format(const char* fmt, Args const&... args)
        {
            const int size = std::snprintf(nullptr, 0, fmt, argument(args)...);
            if (size < 0)
                throw std::system_error(errno, std::generic_category(), "Failed to compute resulting string size");
            // Note: snprintf also writes a trailing null character. We should remove it.
            std::string ret(static_cast<std::size_t>(size) + 1, '\0');
            if (std::snprintf(ret.data(), ret.size(), fmt, argument(args)...) < 0)
                throw std::system_error(errno, std::generic_category(), "Failed to format string");
            ret.erase(static_cast<std::size_t>(size));
            return ret;
        }
    }

    template <typename... Args>
    std::string format(const char* fmt, Args const&... args)
    {
        return Details::format(fmt, Details::nullTerminated(args)...);
    }

    template <typename... Args>
    std::string format(const std::string& fmt, Args const&... args)
    {
        return Details::format(fmt.c_str(), Details::nullTerminated(args)...);
    }
}

#endif
