#ifndef OPENMW_COMPONENTS_VFS_PATHUTIL_H
#define OPENMW_COMPONENTS_VFS_PATHUTIL_H

#include <components/misc/strings/lower.hpp>

#include <algorithm>
#include <ostream>
#include <string>
#include <string_view>

namespace VFS::Path
{
    inline constexpr char normalize(char c)
    {
        return c == '\\' ? '/' : Misc::StringUtils::toLower(c);
    }

    inline void normalizeFilenameInPlace(std::string& name)
    {
        std::transform(name.begin(), name.end(), name.begin(), normalize);
    }

    /// Normalize the given filename, making slashes/backslashes consistent, and lower-casing.
    [[nodiscard]] inline std::string normalizeFilename(std::string_view name)
    {
        std::string out(name);
        normalizeFilenameInPlace(out);
        return out;
    }

    struct PathCharLess
    {
        bool operator()(char x, char y) const { return normalize(x) < normalize(y); }
    };

    inline bool pathLess(std::string_view x, std::string_view y)
    {
        return std::lexicographical_compare(x.begin(), x.end(), y.begin(), y.end(), PathCharLess());
    }

    inline bool pathEqual(std::string_view x, std::string_view y)
    {
        if (std::size(x) != std::size(y))
            return false;
        return std::equal(
            std::begin(x), std::end(x), std::begin(y), [](char l, char r) { return normalize(l) == normalize(r); });
    }

    struct PathLess
    {
        using is_transparent = void;

        bool operator()(std::string_view left, std::string_view right) const { return pathLess(left, right); }
    };

    class Normalized
    {
    public:
        Normalized() = default;

        Normalized(std::string_view value)
            : mValue(normalizeFilename(value))
        {
        }

        Normalized(const char* value)
            : Normalized(std::string_view(value))
        {
        }

        Normalized(const std::string& value)
            : Normalized(std::string_view(value))
        {
        }

        explicit Normalized(std::string&& value)
            : mValue(std::move(value))
        {
            normalizeFilenameInPlace(mValue);
        }

        const std::string& value() const& { return mValue; }

        std::string value() && { return std::move(mValue); }

        std::string_view view() const { return mValue; }

        operator std::string_view() const { return mValue; }

        operator const std::string&() const { return mValue; }

        friend bool operator==(const Normalized& lhs, const Normalized& rhs) = default;

        template <class T>
        friend bool operator==(const Normalized& lhs, const T& rhs)
        {
            return lhs.mValue == rhs;
        }

        friend bool operator<(const Normalized& lhs, const Normalized& rhs) { return lhs.mValue < rhs.mValue; }

        template <class T>
        friend bool operator<(const Normalized& lhs, const T& rhs)
        {
            return lhs.mValue < rhs;
        }

        template <class T>
        friend bool operator<(const T& lhs, const Normalized& rhs)
        {
            return lhs < rhs.mValue;
        }

        friend std::ostream& operator<<(std::ostream& stream, const Normalized& value)
        {
            return stream << value.mValue;
        }

    private:
        std::string mValue;
    };
}

#endif
