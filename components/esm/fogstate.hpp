#ifndef OPENMW_ESM_FOGSTATE_H
#define OPENMW_ESM_FOGSTATE_H

#include <vector>

namespace osg
{
    class Image;
}

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    struct FogTexture
    {
        int mX, mY; // Only used for interior cells
        std::vector<char> mImageData;
        // the dynamic texture is a bottleneck, so don't set this too high
        static const int sFogOfWarResolution = 32;
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

        std::vector<FogTexture> mFogTextures;

        void load (ESMReader &esm);
        void save (ESMWriter &esm, bool interiorCell) const;

        static void saveFogOfWar(osg::Image* fogImage, ESM::FogTexture &fog);
        static osg::Image* loadFogOfWar(const ESM::FogTexture &esm);
        static void convertFogOfWar(std::vector<char>& imageData);
        static osg::Image* initFogOfWar();
    };
}

#endif
