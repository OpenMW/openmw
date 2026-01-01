#include "globalmap.hpp"

#include <osg/Geometry>
#include <osg/Group>
#include <osg/Image>
#include <osg/TexEnvCombine>
#include <osg/Texture2D>

#include <osgDB/WriteFile>

#include <components/files/memorystream.hpp>
#include <components/settings/values.hpp>

#include <components/debug/debuglog.hpp>

#include <components/resource/imagemanager.hpp>
#include <components/resource/resourcesystem.hpp>

#include <components/sceneutil/depth.hpp>
#include <components/sceneutil/nodecallback.hpp>
#include <components/sceneutil/workqueue.hpp>

#include <components/vfs/pathutil.hpp>

#include <components/esm3/globalmap.hpp>
#include <components/esm3/loadland.hpp>

#include "../mwbase/environment.hpp"

#include "../mwworld/esmstore.hpp"

#include "vismask.hpp"

namespace
{

    // Create a screen-aligned quad with given texture coordinates.
    // Assumes a top-left origin of the sampled image.
    osg::ref_ptr<osg::Geometry> createTexturedQuad(
        float leftTexCoord, float topTexCoord, float rightTexCoord, float bottomTexCoord)
    {
        osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;

        osg::ref_ptr<osg::Vec3Array> verts = new osg::Vec3Array;
        verts->push_back(osg::Vec3f(-1, -1, 0));
        verts->push_back(osg::Vec3f(-1, 1, 0));
        verts->push_back(osg::Vec3f(1, 1, 0));
        verts->push_back(osg::Vec3f(1, -1, 0));

        geom->setVertexArray(verts);

        osg::ref_ptr<osg::Vec2Array> texcoords = new osg::Vec2Array;
        texcoords->push_back(osg::Vec2f(leftTexCoord, 1.f - bottomTexCoord));
        texcoords->push_back(osg::Vec2f(leftTexCoord, 1.f - topTexCoord));
        texcoords->push_back(osg::Vec2f(rightTexCoord, 1.f - topTexCoord));
        texcoords->push_back(osg::Vec2f(rightTexCoord, 1.f - bottomTexCoord));

        osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
        colors->push_back(osg::Vec4(1.f, 1.f, 1.f, 1.f));
        geom->setColorArray(colors, osg::Array::BIND_OVERALL);

        geom->setTexCoordArray(0, texcoords, osg::Array::BIND_PER_VERTEX);

        geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 4));

        return geom;
    }

    class CameraUpdateGlobalCallback : public SceneUtil::NodeCallback<CameraUpdateGlobalCallback, osg::Camera*>
    {
    public:
        CameraUpdateGlobalCallback(MWRender::GlobalMap* parent)
            : mRendered(false)
            , mParent(parent)
        {
        }

        void operator()(osg::Camera* node, osg::NodeVisitor* nv)
        {
            if (mRendered)
            {
                if (mParent->copyResult(node, nv->getTraversalNumber()))
                {
                    node->setNodeMask(0);
                    mParent->markForRemoval(node);
                }
                return;
            }

            traverse(node, nv);

            mRendered = true;
        }

    private:
        bool mRendered;
        MWRender::GlobalMap* mParent;
    };

    std::vector<char> writePng(const osg::Image& overlayImage)
    {
        std::ostringstream ostream;
        osgDB::ReaderWriter* readerwriter = osgDB::Registry::instance()->getReaderWriterForExtension("png");
        if (!readerwriter)
        {
            Log(Debug::Error) << "Error: Can't write map overlay: no png readerwriter found";
            return std::vector<char>();
        }

        osgDB::ReaderWriter::WriteResult result = readerwriter->writeImage(overlayImage, ostream);
        if (!result.success())
        {
            Log(Debug::Warning) << "Error: Can't write map overlay: " << result.message() << " code "
                                << result.status();
            return std::vector<char>();
        }

        std::string data = ostream.str();
        return std::vector<char>(data.begin(), data.end());
    }
}

namespace MWRender
{

    class CreateMapWorkItem : public SceneUtil::WorkItem
    {
    public:
        CreateMapWorkItem(int width, int height, int minX, int minY, int maxX, int maxY, int cellSize,
            const MWWorld::Store<ESM::Land>& landStore, osg::ref_ptr<osg::Image> colorLut)
            : mWidth(width)
            , mHeight(height)
            , mMinX(minX)
            , mMinY(minY)
            , mMaxX(maxX)
            , mMaxY(maxY)
            , mCellSize(cellSize)
            , mLandStore(landStore)
            , mColorLut(colorLut)
        {
        }

        void doWork() override
        {
            osg::ref_ptr<osg::Image> image = new osg::Image;
            image->allocateImage(mWidth, mHeight, 1, GL_RGB, GL_UNSIGNED_BYTE);

            osg::ref_ptr<osg::Image> alphaImage = new osg::Image;
            alphaImage->allocateImage(mWidth, mHeight, 1, GL_ALPHA, GL_UNSIGNED_BYTE);

            for (int x = mMinX; x <= mMaxX; ++x)
            {
                for (int y = mMinY; y <= mMaxY; ++y)
                {
                    const ESM::Land* land = mLandStore.search(x, y);

                    if (land != nullptr && (land->mDataTypes & ESM::Land::DATA_VHGT))
                    {
                        land->loadData(ESM::Land::DATA_VHGT);
                        const ESM::Land::LandData* data = land->getLandData(ESM::Land::DATA_VHGT);

                        if (data)
                        {
                            const int vhgtSize = 65;

                            for (int cellY = 0; cellY < mCellSize; ++cellY)
                            {
                                for (int cellX = 0; cellX < mCellSize; ++cellX)
                                {
                                    int vx = (cellX * (vhgtSize - 1)) / mCellSize;
                                    int vy = (cellY * (vhgtSize - 1)) / mCellSize;

                                    float height = data->mHeights[vy * vhgtSize + vx] / 128.0f;

                                    // Convert height to LUT index using the same method as WNAM generation
                                    // Normalize height: positive heights divided by 128, negative by 16
                                    float normalizedHeight = height / (height > 0.0f ? 128.0f : 16.0f);
                                    // Clamp to [-1, 1] range and convert to [0, 255] index
                                    int lutIndex = static_cast<int>(std::clamp(normalizedHeight, -1.0f, 1.0f) * 127.0f) + 128;

                                    int texelX = (x - mMinX) * mCellSize + cellX;
                                    int texelY = (y - mMinY) * mCellSize + cellY;

                                    osg::Vec4 color = mColorLut->getColor(lutIndex, 0);
                                    image->setColor(color, texelX, texelY);

                                    osg::Vec4 alpha(0.0f, 0.0f, 0.0f, lutIndex < 128 ? 0.0f : 1.0f);
                                    alphaImage->setColor(alpha, texelX, texelY);
                                }
                            }
                        }
                    }
                    else
                    {
                        for (int cellY = 0; cellY < mCellSize; ++cellY)
                        {
                            for (int cellX = 0; cellX < mCellSize; ++cellX)
                            {
                                int texelX = (x - mMinX) * mCellSize + cellX;
                                int texelY = (y - mMinY) * mCellSize + cellY;

                                int lutIndex = 0;
                                osg::Vec4 color = mColorLut->getColor(lutIndex, 0);
                                image->setColor(color, texelX, texelY);

                                // Set alpha based on lutIndex threshold
                                osg::Vec4 alpha(0.0f, 0.0f, 0.0f, lutIndex < 128 ? 0.0f : 1.0f);
                                alphaImage->setColor(alpha, texelX, texelY);
                            }
                        }
                    }
                }
            }

            mBaseTexture = new osg::Texture2D;
            mBaseTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            mBaseTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            mBaseTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
            mBaseTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
            mBaseTexture->setImage(image);
            mBaseTexture->setResizeNonPowerOfTwoHint(false);

            mAlphaTexture = new osg::Texture2D;
            mAlphaTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            mAlphaTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            mAlphaTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
            mAlphaTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
            mAlphaTexture->setImage(alphaImage);
            mAlphaTexture->setResizeNonPowerOfTwoHint(false);

            mOverlayImage = new osg::Image;
            mOverlayImage->allocateImage(mWidth, mHeight, 1, GL_RGBA, GL_UNSIGNED_BYTE);
            assert(mOverlayImage->isDataContiguous());

            memset(mOverlayImage->data(), 0, mOverlayImage->getTotalSizeInBytes());

            mOverlayTexture = new osg::Texture2D;
            mOverlayTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            mOverlayTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            mOverlayTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
            mOverlayTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
            mOverlayTexture->setResizeNonPowerOfTwoHint(false);
            mOverlayTexture->setInternalFormat(GL_RGBA);
            mOverlayTexture->setTextureSize(mWidth, mHeight);
        }

        int mWidth, mHeight;
        int mMinX, mMinY, mMaxX, mMaxY;
        int mCellSize;
        const MWWorld::Store<ESM::Land>& mLandStore;
        osg::ref_ptr<osg::Image> mColorLut;

        osg::ref_ptr<osg::Texture2D> mBaseTexture;
        osg::ref_ptr<osg::Texture2D> mAlphaTexture;

        osg::ref_ptr<osg::Image> mOverlayImage;
        osg::ref_ptr<osg::Texture2D> mOverlayTexture;
    };

    struct GlobalMap::WritePng final : public SceneUtil::WorkItem
    {
        osg::ref_ptr<const osg::Image> mOverlayImage;
        std::vector<char> mImageData;

        explicit WritePng(osg::ref_ptr<const osg::Image> overlayImage)
            : mOverlayImage(std::move(overlayImage))
        {
        }

        void doWork() override { mImageData = writePng(*mOverlayImage); }
    };

    GlobalMap::GlobalMap(osg::Group* root, SceneUtil::WorkQueue* workQueue)
        : mRoot(root)
        , mWorkQueue(workQueue)
        , mWidth(0)
        , mHeight(0)
        , mMinX(0)
        , mMaxX(0)
        , mMinY(0)
        , mMaxY(0)
    {
    }

    GlobalMap::~GlobalMap()
    {
        for (auto& camera : mCamerasPendingRemoval)
            removeCamera(camera);
        for (auto& camera : mActiveCameras)
            removeCamera(camera);

        if (mWorkItem)
            mWorkItem->waitTillDone();
    }

    void GlobalMap::render()
    {
        const MWWorld::ESMStore& esmStore = *MWBase::Environment::get().getESMStore();

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

        const int cellSize = Settings::map().mGlobalMapCellSize;

        mWidth = cellSize * (mMaxX - mMinX + 1);
        mHeight = cellSize * (mMaxY - mMinY + 1);

        // Load color LUT texture
        constexpr VFS::Path::NormalizedView colorLutPath("textures/omw_map_color_palette.dds");
        auto resourceSystem = MWBase::Environment::get().getResourceSystem();
        osg::ref_ptr<osg::Image> colorLut = resourceSystem->getImageManager()->getImage(colorLutPath);

        // Validate LUT dimensions
        if (!colorLut || colorLut->s() != 256 || colorLut->t() != 1)
        {
            throw std::runtime_error("Global map color LUT must be 256x1 pixels, got "
                + std::to_string(colorLut ? colorLut->s() : 0) + "x" + std::to_string(colorLut ? colorLut->t() : 0));
        }

        mWorkItem = new CreateMapWorkItem(
            mWidth, mHeight, mMinX, mMinY, mMaxX, mMaxY, cellSize, esmStore.get<ESM::Land>(), colorLut);
        mWorkQueue->addWorkItem(mWorkItem);
    }

    void GlobalMap::worldPosToImageSpace(float x, float z, float& imageX, float& imageY)
    {
        imageX = (float(x / float(Constants::CellSizeInUnits) - mMinX) / (mMaxX - mMinX + 1)) * getWidth();

        imageY = (1.f - float(z / float(Constants::CellSizeInUnits) - mMinY) / (mMaxY - mMinY + 1)) * getHeight();
    }

    void GlobalMap::requestOverlayTextureUpdate(int x, int y, int width, int height,
        osg::ref_ptr<osg::Texture2D> texture, bool clear, bool cpuCopy, float srcLeft, float srcTop, float srcRight,
        float srcBottom)
    {
        osg::ref_ptr<osg::Camera> camera(new osg::Camera);
        camera->setNodeMask(Mask_RenderToTexture);
        camera->setReferenceFrame(osg::Camera::ABSOLUTE_RF);
        camera->setViewMatrix(osg::Matrix::identity());
        camera->setProjectionMatrix(osg::Matrix::identity());
        camera->setProjectionResizePolicy(osg::Camera::FIXED);
        camera->setRenderOrder(osg::Camera::PRE_RENDER, 1); // Make sure the global map is rendered after the local map
        y = mHeight - y - height; // convert top-left origin to bottom-left
        camera->setViewport(x, y, width, height);

        if (clear)
        {
            camera->setClearMask(GL_COLOR_BUFFER_BIT);
            camera->setClearColor(osg::Vec4(0, 0, 0, 0));
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
            osg::ref_ptr<osg::Image> image(new osg::Image);
            image->setPixelFormat(mOverlayImage->getPixelFormat());
            image->setDataType(mOverlayImage->getDataType());
            camera->attach(osg::Camera::COLOR_BUFFER, image);

            ImageDest imageDest;
            imageDest.mImage = image;
            imageDest.mX = x;
            imageDest.mY = y;
            mPendingImageDest[camera] = std::move(imageDest);
        }

        // Create a quad rendering the updated texture
        if (texture)
        {
            osg::ref_ptr<osg::Geometry> geom = createTexturedQuad(srcLeft, srcTop, srcRight, srcBottom);
            osg::ref_ptr<osg::Depth> depth = new SceneUtil::AutoDepth;
            depth->setWriteMask(false);
            osg::StateSet* stateset = geom->getOrCreateStateSet();
            stateset->setAttribute(depth);
            stateset->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
            stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
            stateset->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);

            if (mAlphaTexture)
            {
                osg::ref_ptr<osg::Vec2Array> texcoords = new osg::Vec2Array;

                float x1 = x / static_cast<float>(mWidth);
                float x2 = (x + width) / static_cast<float>(mWidth);
                float y1 = y / static_cast<float>(mHeight);
                float y2 = (y + height) / static_cast<float>(mHeight);
                texcoords->push_back(osg::Vec2f(x1, y1));
                texcoords->push_back(osg::Vec2f(x1, y2));
                texcoords->push_back(osg::Vec2f(x2, y2));
                texcoords->push_back(osg::Vec2f(x2, y1));
                geom->setTexCoordArray(1, texcoords, osg::Array::BIND_PER_VERTEX);

                stateset->setTextureAttributeAndModes(1, mAlphaTexture, osg::StateAttribute::ON);
                osg::ref_ptr<osg::TexEnvCombine> texEnvCombine = new osg::TexEnvCombine;
                texEnvCombine->setCombine_RGB(osg::TexEnvCombine::REPLACE);
                texEnvCombine->setSource0_RGB(osg::TexEnvCombine::PREVIOUS);
                stateset->setTextureAttributeAndModes(1, texEnvCombine);
            }

            camera->addChild(geom);
        }

        mRoot->addChild(camera);

        mActiveCameras.push_back(camera);
    }

    void GlobalMap::exploreCell(int cellX, int cellY, osg::ref_ptr<osg::Texture2D> localMapTexture)
    {
        ensureLoaded();

        if (!localMapTexture)
            return;

        const int cellSize = Settings::map().mGlobalMapCellSize;
        const int originX = (cellX - mMinX) * cellSize;
        // +1 because we want the top left corner of the cell, not the bottom left
        const int originY = (cellY - mMinY + 1) * cellSize;

        if (cellX > mMaxX || cellX < mMinX || cellY > mMaxY || cellY < mMinY)
            return;

        requestOverlayTextureUpdate(
            originX, mHeight - originY, cellSize, cellSize, std::move(localMapTexture), false, true);
    }

    void GlobalMap::clear()
    {
        ensureLoaded();

        memset(mOverlayImage->data(), 0, mOverlayImage->getTotalSizeInBytes());

        mPendingImageDest.clear();

        // just push a Camera to clear the FBO, instead of setImage()/dirty()
        // easier, since we don't need to worry about synchronizing access :)
        requestOverlayTextureUpdate(0, 0, mWidth, mHeight, osg::ref_ptr<osg::Texture2D>(), true, false);
    }

    void GlobalMap::write(ESM::GlobalMap& map)
    {
        ensureLoaded();

        map.mBounds.mMinX = mMinX;
        map.mBounds.mMaxX = mMaxX;
        map.mBounds.mMinY = mMinY;
        map.mBounds.mMaxY = mMaxY;

        if (mWritePng != nullptr)
        {
            mWritePng->waitTillDone();
            map.mImageData = std::move(mWritePng->mImageData);
            mWritePng = nullptr;
            return;
        }

        map.mImageData = writePng(*mOverlayImage);
    }

    struct Box
    {
        int mLeft, mTop, mRight, mBottom;

        Box(int left, int top, int right, int bottom)
            : mLeft(left)
            , mTop(top)
            , mRight(right)
            , mBottom(bottom)
        {
        }
        bool operator==(const Box& other) const
        {
            return mLeft == other.mLeft && mTop == other.mTop && mRight == other.mRight && mBottom == other.mBottom;
        }
    };

    void GlobalMap::read(ESM::GlobalMap& map)
    {
        ensureLoaded();

        const ESM::GlobalMap::Bounds& bounds = map.mBounds;

        if (bounds.mMaxX - bounds.mMinX < 0)
            return;
        if (bounds.mMaxY - bounds.mMinY < 0)
            return;

        if (bounds.mMinX > bounds.mMaxX || bounds.mMinY > bounds.mMaxY)
            throw std::runtime_error("invalid map bounds");

        if (map.mImageData.empty())
            return;

        Files::IMemStream istream(map.mImageData.data(), map.mImageData.size());

        osgDB::ReaderWriter* readerwriter = osgDB::Registry::instance()->getReaderWriterForExtension("png");
        if (!readerwriter)
        {
            Log(Debug::Error) << "Error: Can't read map overlay: no png readerwriter found";
            return;
        }

        osgDB::ReaderWriter::ReadResult result = readerwriter->readImage(istream);
        if (!result.success())
        {
            Log(Debug::Error) << "Error: Can't read map overlay: " << result.message() << " code " << result.status();
            return;
        }

        osg::ref_ptr<osg::Image> image = result.getImage();
        int imageWidth = image->s();
        int imageHeight = image->t();

        int xLength = (bounds.mMaxX - bounds.mMinX + 1);
        int yLength = (bounds.mMaxY - bounds.mMinY + 1);

        // Size of one cell in image space
        int cellImageSizeSrc = imageWidth / xLength;
        if (int(imageHeight / yLength) != cellImageSizeSrc)
            throw std::runtime_error("cell size must be quadratic");

        // If cell bounds of the currently loaded content and the loaded savegame do not match,
        // we need to resize source/dest boxes to accommodate
        // This means nonexisting cells will be dropped silently
        const int cellImageSizeDst = Settings::map().mGlobalMapCellSize;

        // Completely off-screen? -> no need to blit anything
        if (bounds.mMaxX < mMinX || bounds.mMaxY < mMinY || bounds.mMinX > mMaxX || bounds.mMinY > mMaxY)
            return;

        int leftDiff = (mMinX - bounds.mMinX);
        int topDiff = (bounds.mMaxY - mMaxY);
        int rightDiff = (bounds.mMaxX - mMaxX);
        int bottomDiff = (mMinY - bounds.mMinY);

        Box srcBox(std::max(0, leftDiff * cellImageSizeSrc), std::max(0, topDiff * cellImageSizeSrc),
            std::min(imageWidth, imageWidth - rightDiff * cellImageSizeSrc),
            std::min(imageHeight, imageHeight - bottomDiff * cellImageSizeSrc));

        Box destBox(std::max(0, -leftDiff * cellImageSizeDst), std::max(0, -topDiff * cellImageSizeDst),
            std::min(mWidth, mWidth + rightDiff * cellImageSizeDst),
            std::min(mHeight, mHeight + bottomDiff * cellImageSizeDst));

        osg::ref_ptr<osg::Texture2D> texture(new osg::Texture2D);
        texture->setImage(image);
        texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        texture->setResizeNonPowerOfTwoHint(false);

        if (srcBox == destBox && imageWidth == mWidth && imageHeight == mHeight)
        {
            mOverlayImage = image;

            requestOverlayTextureUpdate(0, 0, mWidth, mHeight, std::move(texture), true, false);
        }
        else
        {
            // Dimensions don't match. This could mean a changed map region, or a changed map resolution.
            // In the latter case, we'll want filtering.
            // Create a RTT Camera and draw the image onto mOverlayImage in the next frame.
            requestOverlayTextureUpdate(destBox.mLeft, destBox.mTop, destBox.mRight - destBox.mLeft,
                destBox.mBottom - destBox.mTop, std::move(texture), true, true, srcBox.mLeft / float(imageWidth),
                srcBox.mTop / float(imageHeight), srcBox.mRight / float(imageWidth),
                srcBox.mBottom / float(imageHeight));
        }
    }

    osg::ref_ptr<osg::Texture2D> GlobalMap::getBaseTexture()
    {
        ensureLoaded();
        return mBaseTexture;
    }

    osg::ref_ptr<osg::Texture2D> GlobalMap::getOverlayTexture()
    {
        ensureLoaded();
        return mOverlayTexture;
    }

    void GlobalMap::ensureLoaded()
    {
        if (mWorkItem)
        {
            mWorkItem->waitTillDone();

            mOverlayImage = mWorkItem->mOverlayImage;
            mBaseTexture = mWorkItem->mBaseTexture;
            mAlphaTexture = mWorkItem->mAlphaTexture;
            mOverlayTexture = mWorkItem->mOverlayTexture;

            requestOverlayTextureUpdate(0, 0, mWidth, mHeight, osg::ref_ptr<osg::Texture2D>(), true, false);

            mWorkItem = nullptr;
        }
    }

    bool GlobalMap::copyResult(osg::Camera* camera, unsigned int frame)
    {
        ImageDestMap::iterator it = mPendingImageDest.find(camera);
        if (it == mPendingImageDest.end())
            return true;
        else
        {
            ImageDest& imageDest = it->second;
            if (imageDest.mFrameDone == 0)
                imageDest.mFrameDone
                    = frame + 2; // wait an extra frame to ensure the draw thread has completed its frame.
            if (imageDest.mFrameDone > frame)
            {
                ++it;
                return false;
            }

            mOverlayImage->copySubImage(imageDest.mX, imageDest.mY, 0, imageDest.mImage);
            mPendingImageDest.erase(it);
            return true;
        }
    }

    void GlobalMap::markForRemoval(osg::Camera* camera)
    {
        CameraVector::iterator found = std::find(mActiveCameras.begin(), mActiveCameras.end(), camera);
        if (found == mActiveCameras.end())
        {
            Log(Debug::Error) << "Error: GlobalMap trying to remove an inactive camera";
            return;
        }
        mActiveCameras.erase(found);
        mCamerasPendingRemoval.push_back(camera);
    }

    void GlobalMap::cleanupCameras()
    {
        for (auto& camera : mCamerasPendingRemoval)
            removeCamera(camera);

        mCamerasPendingRemoval.clear();
    }

    void GlobalMap::removeCamera(osg::Camera* cam)
    {
        cam->removeChildren(0, cam->getNumChildren());
        mRoot->removeChild(cam);
    }

    void GlobalMap::asyncWritePng()
    {
        if (mOverlayImage == nullptr)
            return;
        // Use deep copy to avoid any sychronization
        mWritePng = new WritePng(new osg::Image(*mOverlayImage, osg::CopyOp::DEEP_COPY_ALL));
        mWorkQueue->addWorkItem(mWritePng, /*front=*/true);
    }
}
