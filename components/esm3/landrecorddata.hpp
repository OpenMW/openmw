#ifndef OPENMW_COMPONENTS_ESM3_LANDRECORDDATA_H
#define OPENMW_COMPONENTS_ESM3_LANDRECORDDATA_H

#include <array>
#include <cstdint>

namespace ESM
{
    struct LandRecordData
    {
        // number of vertices per side
        static constexpr unsigned sLandSize = 65;

        // total number of vertices
        static constexpr unsigned sLandNumVerts = sLandSize * sLandSize;

        // number of textures per side of land
        static constexpr unsigned sLandTextureSize = 16;

        // total number of textures per land
        static constexpr unsigned sLandNumTextures = sLandTextureSize * sLandTextureSize;

        // Height in world space for each vertex
        std::array<float, sLandNumVerts> mHeights;
        float mMinHeight = 0;
        float mMaxHeight = 0;

        // 24-bit normals, these aren't always correct though. Edge and corner normals may be garbage.
        std::array<std::int8_t, 3 * sLandNumVerts> mNormals;

        // 2D array of texture indices. An index can be used to look up an LandTexture,
        // but to do so you must subtract 1 from the index first!
        // An index of 0 indicates the default texture.
        std::array<std::uint16_t, sLandNumTextures> mTextures;

        // 24-bit RGB color for each vertex
        std::array<std::uint8_t, 3 * sLandNumVerts> mColours;

        int mDataLoaded = 0;
    };
}

#endif
