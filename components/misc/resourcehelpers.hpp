#ifndef MISC_RESOURCEHELPERS_H
#define MISC_RESOURCEHELPERS_H

#include <string>

namespace VFS
{
    class Manager;
}

namespace Misc
{
    // Workarounds for messy resource handling in vanilla morrowind
    // The below functions are provided on a opt-in basis, instead of built into the VFS,
    // so we have the opportunity to use proper resource handling for content created in OpenMW-CS.
    namespace ResourceHelpers
    {
        bool changeExtensionToDds(std::string &path);
        std::string correctResourcePath(const std::string &topLevelDirectory, const std::string &resPath, const VFS::Manager* vfs);
        std::string correctTexturePath(const std::string &resPath, const VFS::Manager* vfs);
        std::string correctIconPath(const std::string &resPath, const VFS::Manager* vfs);
        std::string correctBookartPath(const std::string &resPath, const VFS::Manager* vfs);
        std::string correctBookartPath(const std::string &resPath, int width, int height, const VFS::Manager* vfs);
        /// Use "xfoo.nif" instead of "foo.nif" if available
        std::string correctActorModelPath(const std::string &resPath, const VFS::Manager* vfs);
    }
}

#endif
