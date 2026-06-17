#ifndef OPENMW_ESM_FOGSTATE_H
#define OPENMW_ESM_FOGSTATE_H

#include <cstdint>
#include <vector>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    struct FogTexture
    {
        int32_t mX, mY; // Only used for interior cells
        std::vector<char> mImageData;
    };

    // format 0, saved games only
    // Fog of war state
    struct FogState
    {
        // Only used for interior cells
        float mNorthMarkerAngle;
        struct Bounds
        {
            float mMinX;
            float mMinY;
            float mMaxX;
            float mMaxY;
        } mBounds;
        float mCenterX;
        float mCenterY;

        std::vector<FogTexture> mFogTextures;

        void load(ESMReader& esm);
        void save(ESMWriter& esm, bool interiorCell) const;
    };
}

#endif
