#include "globalmap.hpp"

#include <climits>

#include <osg/Image>
#include <osg/Texture2D>
#include <osg/Group>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Depth>

#include <osgDB/WriteFile>

#include <components/loadinglistener/loadinglistener.hpp>
#include <components/settings/settings.hpp>
#include <components/files/memorystream.hpp>

#include <components/esm/globalmap.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/esmstore.hpp"

#include "vismask.hpp"

namespace
{

    // Create a screen-aligned quad with given texture coordinates.
    // Assumes a top-left origin of the sampled image.
    osg::ref_ptr<osg::Geometry> createTexturedQuad(float leftTexCoord, float topTexCoord, float rightTexCoord, float bottomTexCoord)
    {
        osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;

        osg::ref_ptr<osg::Vec3Array> verts = new osg::Vec3Array;
        verts->push_back(osg::Vec3f(-1, -1, 0));
        verts->push_back(osg::Vec3f(-1, 1, 0));
        verts->push_back(osg::Vec3f(1, 1, 0));
        verts->push_back(osg::Vec3f(1, -1, 0));

        geom->setVertexArray(verts);

        osg::ref_ptr<osg::Vec2Array> texcoords = new osg::Vec2Array;
        texcoords->push_back(osg::Vec2f(leftTexCoord, 1.f-bottomTexCoord));
        texcoords->push_back(osg::Vec2f(leftTexCoord, 1.f-topTexCoord));
        texcoords->push_back(osg::Vec2f(rightTexCoord, 1.f-topTexCoord));
        texcoords->push_back(osg::Vec2f(rightTexCoord, 1.f-bottomTexCoord));

        osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
        colors->push_back(osg::Vec4(1.f, 1.f, 1.f, 1.f));
        geom->setColorArray(colors, osg::Array::BIND_OVERALL);

        geom->setTexCoordArray(0, texcoords, osg::Array::BIND_PER_VERTEX);

        geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,4));

        return geom;
    }


    class CameraUpdateGlobalCallback : public osg::NodeCallback
    {
    public:
        CameraUpdateGlobalCallback(MWRender::GlobalMap* parent)
            : mRendered(false)
            , mParent(parent)
        {
        }

        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            if (mRendered)
            {
                node->setNodeMask(0);
                return;
            }

            traverse(node, nv);

            if (!mRendered)
            {
                mRendered = true;
                mParent->markForRemoval(static_cast<osg::Camera*>(node));
            }
        }

    private:
        bool mRendered;
        MWRender::GlobalMap* mParent;
    };

}

namespace MWRender
{

    GlobalMap::GlobalMap(osg::Group* root)
        : mRoot(root)
        , mWidth(0)
        , mHeight(0)
        , mMinX(0), mMaxX(0)
        , mMinY(0), mMaxY(0)

    {
        mCellSize = Settings::Manager::getInt("global map cell size", "Map");
    }

    GlobalMap::~GlobalMap()
    {
    }

    void GlobalMap::render (Loading::Listener* loadingListener)
    {
        const MWWorld::ESMStore &esmStore =
            MWBase::Environment::get().getWorld()->getStore();

        // get the size of the world
        MWWorld::Store<ESM::Cell>::iterator it = esmStore.get<ESM::Cell>().extBegin();
        for (; it != esmStore.get<ESM::Cell>().extEnd(); ++it)
        {
            if (it->getGridX() < mMinX)
                mMinX = it->getGridX();
            if (it->getGridX() > mMaxX)
                mMaxX = it->getGridX();
            if (it->getGridY() < mMinY)
                mMinY = it->getGridY();
            if (it->getGridY() > mMaxY)
                mMaxY = it->getGridY();
        }

        mWidth = mCellSize*(mMaxX-mMinX+1);
        mHeight = mCellSize*(mMaxY-mMinY+1);

        loadingListener->loadingOn();
        loadingListener->setLabel("Creating map");
        loadingListener->setProgressRange((mMaxX-mMinX+1) * (mMaxY-mMinY+1));
        loadingListener->setProgress(0);

        osg::ref_ptr<osg::Image> image = new osg::Image;
        image->allocateImage(mWidth, mHeight, 1, GL_RGB, GL_UNSIGNED_BYTE);
        unsigned char* data = image->data();

        for (int x = mMinX; x <= mMaxX; ++x)
        {
            for (int y = mMinY; y <= mMaxY; ++y)
            {
                ESM::Land* land = esmStore.get<ESM::Land>().search (x,y);

                if (land)
                {
                    int mask = ESM::Land::DATA_WNAM;
                    if (!land->isDataLoaded(mask))
                        land->loadData(mask);
                }

                const ESM::Land::LandData *landData =
                    land ? land->getLandData (ESM::Land::DATA_WNAM) : 0;

                for (int cellY=0; cellY<mCellSize; ++cellY)
                {
                    for (int cellX=0; cellX<mCellSize; ++cellX)
                    {
                        int vertexX = static_cast<int>(float(cellX)/float(mCellSize) * 9);
                        int vertexY = static_cast<int>(float(cellY) / float(mCellSize) * 9);

                        int texelX = (x-mMinX) * mCellSize + cellX;
                        int texelY = (mHeight-1) - ((y-mMinY) * mCellSize + cellY);

                        unsigned char r,g,b;

                        float y = 0;
                        if (landData)
                            y = (landData->mWnam[vertexY * 9 + vertexX] << 4) / 2048.f;
                        else
                            y = (SCHAR_MIN << 4) / 2048.f;
                        if (y < 0)
                        {
                            r = static_cast<unsigned char>(14 * y + 38);
                            g = static_cast<unsigned char>(20 * y + 56);
                            b = static_cast<unsigned char>(18 * y + 51);
                        }
                        else if (y < 0.3f)
                        {
                            if (y < 0.1f)
                                y *= 8.f;
                            else
                            {
                                y -= 0.1f;
                                y += 0.8f;
                            }
                            r = static_cast<unsigned char>(66 - 32 * y);
                            g = static_cast<unsigned char>(48 - 23 * y);
                            b = static_cast<unsigned char>(33 - 16 * y);
                        }
                        else
                        {
                            y -= 0.3f;
                            y *= 1.428f;
                            r = static_cast<unsigned char>(34 - 29 * y);
                            g = static_cast<unsigned char>(25 - 20 * y);
                            b = static_cast<unsigned char>(17 - 12 * y);
                        }

                        data[texelY * mWidth * 3 + texelX * 3] = r;
                        data[texelY * mWidth * 3 + texelX * 3+1] = g;
                        data[texelY * mWidth * 3 + texelX * 3+2] = b;
                    }
                }
                loadingListener->increaseProgress();
                if (land)
                    land->unloadData();
            }
        }

        mBaseTexture = new osg::Texture2D;
        mBaseTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        mBaseTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        mBaseTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        mBaseTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        mBaseTexture->setImage(image);
        mBaseTexture->setResizeNonPowerOfTwoHint(false);

        clear();

        loadingListener->loadingOff();
    }

    void GlobalMap::worldPosToImageSpace(float x, float z, float& imageX, float& imageY)
    {
        imageX = float(x / 8192.f - mMinX) / (mMaxX - mMinX + 1);

        imageY = 1.f-float(z / 8192.f - mMinY) / (mMaxY - mMinY + 1);
    }

    void GlobalMap::cellTopLeftCornerToImageSpace(int x, int y, float& imageX, float& imageY)
    {
        imageX = float(x - mMinX) / (mMaxX - mMinX + 1);

        // NB y + 1, because we want the top left corner, not bottom left where the origin of the cell is
        imageY = 1.f-float(y - mMinY + 1) / (mMaxY - mMinY + 1);
    }

    void GlobalMap::requestOverlayTextureUpdate(int x, int y, int width, int height, osg::ref_ptr<osg::Texture2D> texture, bool clear, bool cpuCopy,
                                                float srcLeft, float srcTop, float srcRight, float srcBottom)
    {
        osg::ref_ptr<osg::Camera> camera (new osg::Camera);
        camera->setNodeMask(Mask_RenderToTexture);
        camera->setReferenceFrame(osg::Camera::ABSOLUTE_RF);
        camera->setViewMatrix(osg::Matrix::identity());
        camera->setProjectionMatrix(osg::Matrix::identity());
        camera->setProjectionResizePolicy(osg::Camera::FIXED);
        camera->setRenderOrder(osg::Camera::PRE_RENDER);
        camera->setViewport(x, y, width, height);

        if (clear)
        {
            camera->setClearMask(GL_COLOR_BUFFER_BIT);
            camera->setClearColor(osg::Vec4(0,0,0,0));
        }
        else
            camera->setClearMask(GL_NONE);

        camera->setUpdateCallback(new CameraUpdateGlobalCallback(this));

        camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT, osg::Camera::PIXEL_BUFFER_RTT);
        camera->attach(osg::Camera::COLOR_BUFFER, mOverlayTexture);

        // no need for a depth buffer
        camera->setImplicitBufferAttachmentMask(osg::DisplaySettings::IMPLICIT_COLOR_BUFFER_ATTACHMENT);

        if (cpuCopy)
        {
            // Attach an image to copy the render back to the CPU when finished
            osg::ref_ptr<osg::Image> image (new osg::Image);
            image->setPixelFormat(mOverlayImage->getPixelFormat());
            image->setDataType(mOverlayImage->getDataType());
            camera->attach(osg::Camera::COLOR_BUFFER, image);

            ImageDest imageDest;
            imageDest.mImage = image;
            imageDest.mX = x;
            imageDest.mY = y;
            mPendingImageDest.push_back(imageDest);
        }

        // Create a quad rendering the updated texture
        if (texture)
        {
            osg::ref_ptr<osg::Geometry> geom = createTexturedQuad(srcLeft, srcTop, srcRight, srcBottom);
            osg::ref_ptr<osg::Depth> depth = new osg::Depth;
            depth->setWriteMask(0);
            osg::StateSet* stateset = geom->getOrCreateStateSet();
            stateset->setAttribute(depth);
            stateset->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
            stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
            stateset->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
            osg::ref_ptr<osg::Geode> geode = new osg::Geode;
            geode->addDrawable(geom);
            camera->addChild(geode);
        }

        mRoot->addChild(camera);

        mActiveCameras.push_back(camera);
    }

    void GlobalMap::exploreCell(int cellX, int cellY, osg::ref_ptr<osg::Texture2D> localMapTexture)
    {
        if (!localMapTexture)
            return;

        int originX = (cellX - mMinX) * mCellSize;
        int originY = (cellY - mMinY) * mCellSize;

        if (cellX > mMaxX || cellX < mMinX || cellY > mMaxY || cellY < mMinY)
            return;

        requestOverlayTextureUpdate(originX, originY, mCellSize, mCellSize, localMapTexture, false, true);
    }

    void GlobalMap::clear()
    {
        if (!mOverlayImage)
        {
            mOverlayImage = new osg::Image;
            mOverlayImage->allocateImage(mWidth, mHeight, 1, GL_RGBA, GL_UNSIGNED_BYTE);
            assert(mOverlayImage->isDataContiguous());
        }
        memset(mOverlayImage->data(), 0, mOverlayImage->getTotalSizeInBytes());

        if (!mOverlayTexture)
        {
            mOverlayTexture = new osg::Texture2D;
            mOverlayTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            mOverlayTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            mOverlayTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
            mOverlayTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
            mOverlayTexture->setResizeNonPowerOfTwoHint(false);
            mOverlayTexture->setInternalFormat(GL_RGBA);
            mOverlayTexture->setTextureSize(mWidth, mHeight);
        }

        mPendingImageDest.clear();

        // just push a Camera to clear the FBO, instead of setImage()/dirty()
        // easier, since we don't need to worry about synchronizing access :)
        requestOverlayTextureUpdate(0, 0, mWidth, mHeight, osg::ref_ptr<osg::Texture2D>(), true, false);
    }

    void GlobalMap::write(ESM::GlobalMap& map)
    {
        map.mBounds.mMinX = mMinX;
        map.mBounds.mMaxX = mMaxX;
        map.mBounds.mMinY = mMinY;
        map.mBounds.mMaxY = mMaxY;

        std::ostringstream ostream;
        osgDB::ReaderWriter* readerwriter = osgDB::Registry::instance()->getReaderWriterForExtension("png");
        if (!readerwriter)
        {
            std::cerr << "Can't write map overlay: no png readerwriter found" << std::endl;
            return;
        }

        osgDB::ReaderWriter::WriteResult result = readerwriter->writeImage(*mOverlayImage, ostream);
        if (!result.success())
        {
            std::cerr << "Can't write map overlay: " << result.message() << " code " << result.status() << std::endl;
            return;
        }

        std::string data = ostream.str();
        map.mImageData = std::vector<char>(data.begin(), data.end());
    }

    struct Box
    {
        int mLeft, mTop, mRight, mBottom;

        Box(int left, int top, int right, int bottom)
            : mLeft(left), mTop(top), mRight(right), mBottom(bottom)
        {
        }
        bool operator == (const Box& other)
        {
            return mLeft == other.mLeft && mTop == other.mTop && mRight == other.mRight && mBottom == other.mBottom;
        }
    };

    void GlobalMap::read(ESM::GlobalMap& map)
    {
        const ESM::GlobalMap::Bounds& bounds = map.mBounds;

        if (bounds.mMaxX-bounds.mMinX < 0)
            return;
        if (bounds.mMaxY-bounds.mMinY < 0)
            return;

        if (bounds.mMinX > bounds.mMaxX
                || bounds.mMinY > bounds.mMaxY)
            throw std::runtime_error("invalid map bounds");

        if (!map.mImageData.size())
            return;

        Files::IMemStream istream(&map.mImageData[0], map.mImageData.size());

        osgDB::ReaderWriter* readerwriter = osgDB::Registry::instance()->getReaderWriterForExtension("png");
        if (!readerwriter)
        {
            std::cerr << "Can't read map overlay: no png readerwriter found" << std::endl;
            return;
        }

        osgDB::ReaderWriter::ReadResult result = readerwriter->readImage(istream);
        if (!result.success())
        {
            std::cerr << "Can't read map overlay: " << result.message() << " code " << result.status() << std::endl;
            return;
        }

        osg::ref_ptr<osg::Image> image = result.getImage();
        int imageWidth = image->s();
        int imageHeight = image->t();

        int xLength = (bounds.mMaxX-bounds.mMinX+1);
        int yLength = (bounds.mMaxY-bounds.mMinY+1);

        // Size of one cell in image space
        int cellImageSizeSrc = imageWidth / xLength;
        if (int(imageHeight / yLength) != cellImageSizeSrc)
            throw std::runtime_error("cell size must be quadratic");

        // If cell bounds of the currently loaded content and the loaded savegame do not match,
        // we need to resize source/dest boxes to accommodate
        // This means nonexisting cells will be dropped silently
        int cellImageSizeDst = mCellSize;

        // Completely off-screen? -> no need to blit anything
        if (bounds.mMaxX < mMinX
                || bounds.mMaxY < mMinY
                || bounds.mMinX > mMaxX
                || bounds.mMinY > mMaxY)
            return;

        int leftDiff = (mMinX - bounds.mMinX);
        int topDiff = (bounds.mMaxY - mMaxY);
        int rightDiff = (bounds.mMaxX - mMaxX);
        int bottomDiff =  (mMinY - bounds.mMinY);

        Box srcBox ( std::max(0, leftDiff * cellImageSizeSrc),
                                  std::max(0, topDiff * cellImageSizeSrc),
                                  std::min(imageWidth, imageWidth - rightDiff * cellImageSizeSrc),
                                  std::min(imageHeight, imageHeight - bottomDiff * cellImageSizeSrc));

        Box destBox ( std::max(0, -leftDiff * cellImageSizeDst),
                                   std::max(0, -topDiff * cellImageSizeDst),
                                   std::min(mWidth, mWidth + rightDiff * cellImageSizeDst),
                                   std::min(mHeight, mHeight + bottomDiff * cellImageSizeDst));

        osg::ref_ptr<osg::Texture2D> texture (new osg::Texture2D);
        texture->setImage(image);
        texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        texture->setResizeNonPowerOfTwoHint(false);

        if (srcBox == destBox && imageWidth == mWidth && imageHeight == mHeight)
        {
            mOverlayImage->copySubImage(0, 0, 0, image);

            requestOverlayTextureUpdate(0, 0, mWidth, mHeight, texture, true, false);
        }
        else
        {
            // Dimensions don't match. This could mean a changed map region, or a changed map resolution.
            // In the latter case, we'll want filtering.
            // Create a RTT Camera and draw the image onto mOverlayImage in the next frame.
            requestOverlayTextureUpdate(destBox.mLeft, destBox.mTop, destBox.mRight-destBox.mLeft, destBox.mBottom-destBox.mTop, texture, true, true,
                                        srcBox.mLeft/float(imageWidth), srcBox.mTop/float(imageHeight),
                                        srcBox.mRight/float(imageWidth), srcBox.mBottom/float(imageHeight));
        }
    }

    osg::ref_ptr<osg::Texture2D> GlobalMap::getBaseTexture()
    {
        return mBaseTexture;
    }

    osg::ref_ptr<osg::Texture2D> GlobalMap::getOverlayTexture()
    {
        return mOverlayTexture;
    }

    void GlobalMap::markForRemoval(osg::Camera *camera)
    {
        CameraVector::iterator found = std::find(mActiveCameras.begin(), mActiveCameras.end(), camera);
        if (found == mActiveCameras.end())
        {
            std::cerr << "GlobalMap trying to remove an inactive camera" << std::endl;
            return;
        }
        mActiveCameras.erase(found);
        mCamerasPendingRemoval.push_back(camera);
    }

    void GlobalMap::cleanupCameras()
    {
        for (CameraVector::iterator it = mCamerasPendingRemoval.begin(); it != mCamerasPendingRemoval.end(); ++it)
            mRoot->removeChild(*it);
        mCamerasPendingRemoval.clear();

        for (ImageDestVector::iterator it = mPendingImageDest.begin(); it != mPendingImageDest.end();)
        {
            ImageDest& imageDest = *it;
            if (--imageDest.mFramesUntilDone > 0)
            {
                ++it;
                continue;
            }

            mOverlayImage->copySubImage(imageDest.mX, imageDest.mY, 0, imageDest.mImage);

            it = mPendingImageDest.erase(it);
        }
    }
}
