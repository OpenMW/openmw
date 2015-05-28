#ifndef GAME_RENDER_GLOBALMAP_H
#define GAME_RENDER_GLOBALMAP_H

#include <string>
#include <vector>

#include <osg/ref_ptr>

namespace osg
{
    class Texture2D;
    class Image;
}

namespace Loading
{
    class Listener;
}

namespace ESM
{
    struct GlobalMap;
}

namespace MWRender
{

    class GlobalMap
    {
    public:
        GlobalMap();
        ~GlobalMap();

        void render(Loading::Listener* loadingListener);

        int getWidth() const { return mWidth; }
        int getHeight() const { return mHeight; }

        int getCellSize() const { return mCellSize; }

        void worldPosToImageSpace(float x, float z, float& imageX, float& imageY);

        void cellTopLeftCornerToImageSpace(int x, int y, float& imageX, float& imageY);

        void exploreCell (int cellX, int cellY);

        /// Clears the overlay
        void clear();

        void write (ESM::GlobalMap& map);
        void read (ESM::GlobalMap& map);

        osg::ref_ptr<osg::Texture2D> getBaseTexture();
        osg::ref_ptr<osg::Texture2D> getOverlayTexture();

    private:
        int mCellSize;

        std::vector< std::pair<int,int> > mExploredCells;

        osg::ref_ptr<osg::Texture2D> mBaseTexture;
        osg::ref_ptr<osg::Texture2D> mOverlayTexture;

        osg::ref_ptr<osg::Image> mOverlayImage;

        int mWidth;
        int mHeight;

        int mMinX, mMaxX, mMinY, mMaxY;
    };

}

#endif

