#ifndef OPENMW_COMPONENTS_ESM3_LANDRECORDDATA_H
#define OPENMW_COMPONENTS_ESM3_LANDRECORDDATA_H

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

        // Initial reference height for the first vertex, only needed for filling mHeights
        float mHeightOffset = 0;
        // Height in world space for each vertex
        float mHeights[sLandNumVerts];
        float mMinHeight = 0;
        float mMaxHeight = 0;

        // 24-bit normals, these aren't always correct though. Edge and corner normals may be garbage.
        std::int8_t mNormals[sLandNumVerts * 3];

        // 2D array of texture indices. An index can be used to look up an LandTexture,
        // but to do so you must subtract 1 from the index first!
        // An index of 0 indicates the default texture.
        std::uint16_t mTextures[sLandNumTextures];

        // 24-bit RGB color for each vertex
        std::uint8_t mColours[3 * sLandNumVerts];

        // ???
        std::uint16_t mUnk1 = 0;
        std::uint8_t mUnk2 = 0;

        int mDataLoaded = 0;
    };
}

#endif
