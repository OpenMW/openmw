#ifndef GAME_RENDER_GLOBALMAP_H
#define GAME_RENDER_GLOBALMAP_H

#include <string>

#include <OgreTexture.h>

namespace Loading
{
    class Listener;
}

namespace ESM
{
    class GlobalMap;
}

namespace MWRender
{

    class GlobalMap : public Ogre::ManualResourceLoader
    {
    public:
        GlobalMap(const std::string& cacheDir);
        ~GlobalMap();

        void render(Loading::Listener* loadingListener);

        int getWidth() { return mWidth; }
        int getHeight() { return mHeight; }

        void worldPosToImageSpace(float x, float z, float& imageX, float& imageY);

        void cellTopLeftCornerToImageSpace(int x, int y, float& imageX, float& imageY);

        void exploreCell (int cellX, int cellY);

        virtual void loadResource(Ogre::Resource* resource);

        /// Clears the overlay
        void clear();

        void write (ESM::GlobalMap& map);
        void read (ESM::GlobalMap& map);

    private:
        std::string mCacheDir;

        std::vector< std::pair<int,int> > mExploredCells;

        Ogre::TexturePtr mOverlayTexture;
        Ogre::Image mOverlayImage; // Backup in system memory

        int mWidth;
        int mHeight;

        int mMinX, mMaxX, mMinY, mMaxY;
    };

}

#endif

