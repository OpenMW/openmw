#ifndef MISC_RESOURCEHELPERS_H
#define MISC_RESOURCEHELPERS_H

#include <string>

namespace VFS
{
    class Manager;
}

namespace Misc
{
    namespace ResourceHelpers
    {
        bool changeExtensionToDds(std::string &path);
        std::string correctResourcePath(const std::string &topLevelDirectory, const std::string &resPath, const VFS::Manager* vfs);
        std::string correctTexturePath(const std::string &resPath, const VFS::Manager* vfs);
        std::string correctIconPath(const std::string &resPath, const VFS::Manager* vfs);
        std::string correctBookartPath(const std::string &resPath, const VFS::Manager* vfs);
        std::string correctBookartPath(const std::string &resPath, int width, int height, const VFS::Manager* vfs);
        /// Uses "xfoo.nif" instead of "foo.nif" if available
        std::string correctActorModelPath(const std::string &resPath, const VFS::Manager* vfs);
    }
}

#endif
