#ifndef COMPONENTS_TERRAIN_DEFS_HPP
#define COMPONENTS_TERRAIN_DEFS_HPP

#include <components/vfs/pathutil.hpp>

namespace Terrain
{

    enum Direction
    {
        North = 0,
        East = 1,
        South = 2,
        West = 3
    };

    struct LayerInfo
    {
        VFS::Path::Normalized mDiffuseMap;
        VFS::Path::Normalized mNormalMap;
        bool mParallax; // Height info in normal map alpha channel?
        bool mSpecular; // Specular info in diffuse map alpha channel?

        bool requiresShaders() const { return !mNormalMap.empty() || mSpecular; }
    };

}

#endif
