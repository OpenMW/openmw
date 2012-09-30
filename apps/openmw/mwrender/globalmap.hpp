#ifndef _GAME_RENDER_GLOBALMAP_H
#define _GAME_RENDER_GLOBALMAP_H

#include <string>

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

    private:
        std::string mCacheDir;

        int mWidth;
        int mHeight;

        int mMinX, mMaxX, mMinY, mMaxY;
    };

}

#endif

