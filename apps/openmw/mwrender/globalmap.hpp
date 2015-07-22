#ifndef GAME_RENDER_GLOBALMAP_H
#define GAME_RENDER_GLOBALMAP_H

#include <string>
#include <vector>

#include <osg/ref_ptr>

namespace osg
{
    class Texture2D;
    class Image;
    class Group;
    class Camera;
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
        GlobalMap(osg::Group* root);
        ~GlobalMap();

        void render(Loading::Listener* loadingListener);

        int getWidth() const { return mWidth; }
        int getHeight() const { return mHeight; }

        int getCellSize() const { return mCellSize; }

        void worldPosToImageSpace(float x, float z, float& imageX, float& imageY);

        void cellTopLeftCornerToImageSpace(int x, int y, float& imageX, float& imageY);

        void exploreCell (int cellX, int cellY, osg::ref_ptr<osg::Texture2D> localMapTexture);

        /// Clears the overlay
        void clear();

        /**
         * Removes cameras that have already been rendered. Should be called every frame to ensure that
         * we do not render the same map more than once. Note, this cleanup is difficult to implement in an
         * automated fashion, since we can't alter the scene graph structure from within an update callback.
         */
        void cleanupCameras();

        /**
         * Mark a camera for cleanup in the next update. For internal use only.
         */
        void markForRemoval(osg::Camera* camera);

        void write (ESM::GlobalMap& map);
        void read (ESM::GlobalMap& map);

        osg::ref_ptr<osg::Texture2D> getBaseTexture();
        osg::ref_ptr<osg::Texture2D> getOverlayTexture();

    private:
        /**
         * Request rendering a 2d quad onto mOverlayTexture.
         * x, y, width and height are the destination coordinates.
         * @param cpuCopy copy the resulting render onto mOverlayImage as well?
         */
        void requestOverlayTextureUpdate(int x, int y, int width, int height, osg::ref_ptr<osg::Texture2D> texture, bool clear, bool cpuCopy,
                                         float srcLeft = 0.f, float srcTop = 0.f, float srcRight = 1.f, float srcBottom = 1.f);

        int mCellSize;

        osg::ref_ptr<osg::Group> mRoot;

        typedef std::vector<osg::ref_ptr<osg::Camera> > CameraVector;
        CameraVector mActiveCameras;

        CameraVector mCamerasPendingRemoval;

        struct ImageDest
        {
            ImageDest()
                : mX(0), mY(0)
                , mFramesUntilDone(3) // wait an extra frame to ensure the draw thread has completed its frame.
            {
            }

            osg::ref_ptr<osg::Image> mImage;
            int mX, mY;
            int mFramesUntilDone;
        };

        typedef std::vector<ImageDest> ImageDestVector;

        ImageDestVector mPendingImageDest;

        std::vector< std::pair<int,int> > mExploredCells;

        osg::ref_ptr<osg::Texture2D> mBaseTexture;

        // GPU copy of overlay
        // Note, uploads are pushed through a Camera, instead of through mOverlayImage
        osg::ref_ptr<osg::Texture2D> mOverlayTexture;

        // CPU copy of overlay
        osg::ref_ptr<osg::Image> mOverlayImage;

        int mWidth;
        int mHeight;

        int mMinX, mMaxX, mMinY, mMaxY;
    };

}

#endif

