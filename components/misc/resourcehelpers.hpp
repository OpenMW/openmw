#ifndef MISC_RESOURCEHELPERS_H
#define MISC_RESOURCEHELPERS_H

#include <string>

namespace Misc
{
    namespace ResourceHelpers
    {
        bool changeExtensionToDds(std::string &path);
        std::string correctResourcePath(const std::string &topLevelDirectory, const std::string &resPath);
        std::string correctTexturePath(const std::string &resPath);
        std::string correctIconPath(const std::string &resPath);
        std::string correctBookartPath(const std::string &resPath);
        std::string correctBookartPath(const std::string &resPath, int width, int height);
        /// Uses "xfoo.nif" instead of "foo.nif" if available
        std::string correctActorModelPath(const std::string &resPath);
    }
}

#endif
