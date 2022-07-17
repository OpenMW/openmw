#ifndef OPENMW_COMPONENTS_MISC_PATHHELPERS_H
#define OPENMW_COMPONENTS_MISC_PATHHELPERS_H

#include <string>

namespace Misc
{
    inline std::string_view getFileExtension(std::string_view file)
    {
        if (auto extPos = file.find_last_of('.'); extPos != std::string::npos)
        {
            file.remove_prefix(extPos + 1);
            return file;
        }
        return {};
    }

    inline std::string_view getFileName(std::string_view path)
    {
        if (auto namePos = path.find_last_of("/\\"); namePos != std::string::npos)
        {
            path.remove_prefix(namePos + 1);
        }

        return path;
    }

    inline std::string_view stemFile(std::string_view path)
    {
        path = getFileName(path);

        if (auto extPos = path.find_last_of("."); extPos != std::string::npos)
        {
            path.remove_suffix(path.size() - extPos);
        }

        return path;
    }
}

#endif
