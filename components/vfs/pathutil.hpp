#ifndef OPENMW_COMPONENTS_RESOURCE_PATH_H
#define OPENMW_COMPONENTS_RESOURCE_PATH_H

#include <components/misc/strings/lower.hpp>

#include <algorithm>
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
}

#endif
