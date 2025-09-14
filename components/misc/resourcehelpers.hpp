#ifndef MISC_RESOURCEHELPERS_H
#define MISC_RESOURCEHELPERS_H

#include <span>
#include <string>
#include <string_view>

#include <components/vfs/pathutil.hpp>

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
        VFS::Path::Normalized correctResourcePath(std::span<const VFS::Path::NormalizedView> topLevelDirectories,
            VFS::Path::NormalizedView resPath, const VFS::Manager& vfs, VFS::Path::ExtensionView ext = {});
        VFS::Path::Normalized correctTexturePath(VFS::Path::NormalizedView resPath, const VFS::Manager& vfs);
        VFS::Path::Normalized correctIconPath(VFS::Path::NormalizedView resPath, const VFS::Manager& vfs);
        VFS::Path::Normalized correctBookartPath(VFS::Path::NormalizedView resPath, const VFS::Manager& vfs);
        VFS::Path::Normalized correctBookartPath(
            VFS::Path::NormalizedView resPath, int width, int height, const VFS::Manager& vfs);
        /// Use "xfoo.nif" instead of "foo.nif" if "xfoo.kf" is available
        /// Note that if "xfoo.nif" is actually unavailable, we can't fall back to "foo.nif". :(
        VFS::Path::Normalized correctActorModelPath(VFS::Path::NormalizedView resPath, const VFS::Manager* vfs);
        VFS::Path::Normalized correctMaterialPath(VFS::Path::NormalizedView resPath, const VFS::Manager& vfs);

        // Prepends "meshes/".
        VFS::Path::Normalized correctMeshPath(VFS::Path::NormalizedView resPath);

        // Prepends "sound/".
        VFS::Path::Normalized correctSoundPath(VFS::Path::NormalizedView resPath);

        // Prepends "music/".
        VFS::Path::Normalized correctMusicPath(VFS::Path::NormalizedView resPath);

        // Removes "meshes\\".
        std::string_view meshPathForESM3(std::string_view resPath);

        VFS::Path::Normalized correctSoundPath(VFS::Path::NormalizedView resPath, const VFS::Manager& vfs);

        /// marker objects that have a hardcoded function in the game logic, should be hidden from the player
        bool isHiddenMarker(const ESM::RefId& id);

        VFS::Path::Normalized getLODMeshName(
            int esmVersion, VFS::Path::NormalizedView resPath, const VFS::Manager& vfs, unsigned char lod = 0);
    }
}

#endif
