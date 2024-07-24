#ifndef OPENMW_COMPONENTS_MISC_PATHHELPERS_H
#define OPENMW_COMPONENTS_MISC_PATHHELPERS_H

#include <string_view>

namespace Misc
{
    inline constexpr std::size_t findExtension(std::string_view file) noexcept
    {
        return file.find_last_of('.');
    }

    inline constexpr std::string_view getFileExtension(std::string_view file) noexcept
    {
        if (auto extPos = findExtension(file); extPos != std::string_view::npos)
        {
            file.remove_prefix(extPos + 1);
            return file;
        }
        return {};
    }

    inline constexpr std::string_view getFileName(std::string_view path) noexcept
    {
        if (auto namePos = path.find_last_of("/\\"); namePos != std::string_view::npos)
        {
            path.remove_prefix(namePos + 1);
        }

        return path;
    }

    inline constexpr std::string_view stemFile(std::string_view path) noexcept
    {
        path = getFileName(path);

        if (auto extPos = path.find_last_of('.'); extPos != std::string_view::npos)
        {
            path.remove_suffix(path.size() - extPos);
        }

        return path;
    }
}

#endif
