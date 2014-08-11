#ifndef MISC_RESOURCEHELPERS_H
#define MISC_RESOURCEHELPERS_H

#include <string>

namespace Misc
{
    namespace ResourceHelpers
    {
        std::string correctResourcePath(const std::string &topLevelDirectory, const std::string &filename);
        std::string correctTexturePath(const std::string &filename);
        std::string correctIconPath(const std::string &filename);
    }
}

#endif
