#ifndef GAME_RENDER_GLOBALMAP_H
#define GAME_RENDER_GLOBALMAP_H

#include <string>
#include <vector>
#include <map>

#include <osg/ref_ptr>

namespace osg
{
    class Texture2D;
    class Image;
    class Group;
    class Camera;
}

namespace ESM
{
    struct GlobalMap;
}

namespace SceneUtil
{
    class WorkQueue;
}

namespace MWRender
{

    class CreateMapWorkItem;

    class GlobalMap
    {
    public:
        GlobalMap(osg::Group* root, SceneUtil::WorkQueue* workQueue);
        ~GlobalMap();

        void render();

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

        void removeCamera(osg::Camera* cam);

        bool copyResult(osg::Camera* cam, unsigned int frame);

        /**
         * Mark a camera for cleanup in the next update. For internal use only.
         */
        void markForRemoval(osg::Camera* camera);

        void write (ESM::GlobalMap& map);
        void read (ESM::GlobalMap& map);

        osg::ref_ptr<osg::Texture2D> getBaseTexture();
        osg::ref_ptr<osg::Texture2D> getOverlayTexture();

        void ensureLoaded();

    private:
        /**
         * Request rendering a 2d quad onto mOverlayTexture.
         * x, y, width and height are the destination coordinates (top-left coordinate origin)
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
                , mFrameDone(0)
            {
            }

            osg::ref_ptr<osg::Image> mImage;
            int mX, mY;
            unsigned int mFrameDone;
        };

        typedef std::map<osg::ref_ptr<osg::Camera>, ImageDest> ImageDestMap;

        ImageDestMap mPendingImageDest;

        std::vector< std::pair<int,int> > mExploredCells;

        osg::ref_ptr<osg::Texture2D> mBaseTexture;
        osg::ref_ptr<osg::Texture2D> mAlphaTexture;

        // GPU copy of overlay
        // Note, uploads are pushed through a Camera, instead of through mOverlayImage
        osg::ref_ptr<osg::Texture2D> mOverlayTexture;

        // CPU copy of overlay
        osg::ref_ptr<osg::Image> mOverlayImage;

        osg::ref_ptr<SceneUtil::WorkQueue> mWorkQueue;
        osg::ref_ptr<CreateMapWorkItem> mWorkItem;

        int mWidth;
        int mHeight;

        int mMinX, mMaxX, mMinY, mMaxY;
    };

}

#endif

