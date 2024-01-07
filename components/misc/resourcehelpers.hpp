#ifndef MISC_RESOURCEHELPERS_H
#define MISC_RESOURCEHELPERS_H

#include <string>
#include <string_view>

namespace VFS
{
    class Manager;
}

namespace ESM
{
    class RefId;
}

namespace Misc
{
    // Workarounds for messy resource handling in vanilla morrowind
    // The below functions are provided on a opt-in basis, instead of built into the VFS,
    // so we have the opportunity to use proper resource handling for content created in OpenMW-CS.
    namespace ResourceHelpers
    {
        bool changeExtensionToDds(std::string& path);
        std::string correctResourcePath(
            std::string_view topLevelDirectory, std::string_view resPath, const VFS::Manager* vfs);
        std::string correctTexturePath(std::string_view resPath, const VFS::Manager* vfs);
        std::string correctIconPath(std::string_view resPath, const VFS::Manager* vfs);
        std::string correctBookartPath(std::string_view resPath, const VFS::Manager* vfs);
        std::string correctBookartPath(std::string_view resPath, int width, int height, const VFS::Manager* vfs);
        /// Use "xfoo.nif" instead of "foo.nif" if "xfoo.kf" is available
        /// Note that if "xfoo.nif" is actually unavailable, we can't fall back to "foo.nif". :(
        std::string correctActorModelPath(const std::string& resPath, const VFS::Manager* vfs);

        // Adds "meshes\\".
        std::string correctMeshPath(std::string_view resPath);

        // Adds "sound\\".
        std::string correctSoundPath(const std::string& resPath);

        // Adds "music\\".
        std::string correctMusicPath(const std::string& resPath);

        // Removes "meshes\\".
        std::string_view meshPathForESM3(std::string_view resPath);

        std::string correctSoundPath(std::string_view resPath, const VFS::Manager* vfs);

        /// marker objects that have a hardcoded function in the game logic, should be hidden from the player
        bool isHiddenMarker(const ESM::RefId& id);
        std::string getLODMeshName(int esmVersion, std::string resPath, const VFS::Manager* vfs, unsigned char lod = 0);
    }
}

#endif
