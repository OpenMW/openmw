#ifndef _GAME_RENDER_GLOBALMAP_H
#define _GAME_RENDER_GLOBALMAP_H

#include <string>

#include <OgreTexture.h>

namespace MWRender
{

    class GlobalMap
    {
    public:
        GlobalMap(const std::string& cacheDir);

        void render();

        int getWidth() { return mWidth; }
        int getHeight() { return mHeight; }

        void worldPosToImageSpace(float x, float z, float& imageX, float& imageY);
        ///< @param x x ogre coords
        /// @param z z ogre coords

        void cellTopLeftCornerToImageSpace(int x, int y, float& imageX, float& imageY);

        void exploreCell (int cellX, int cellY);

    private:
        std::string mCacheDir;

        std::vector< std::pair<int,int> > mExploredCells;

        Ogre::TexturePtr mOverlayTexture;
        std::vector<Ogre::uchar> mExploredBuffer;

        int mWidth;
        int mHeight;

        int mMinX, mMaxX, mMinY, mMaxY;
    };

}

#endif

