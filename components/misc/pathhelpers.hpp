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
}

#endif
