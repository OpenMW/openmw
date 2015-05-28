#include "globalmap.hpp"

#include <climits>

#include <osg/Image>
#include <osg/Texture2D>

#include <osgDB/WriteFile>

#include <components/loadinglistener/loadinglistener.hpp>
#include <components/settings/settings.hpp>
#include <components/files/memorystream.hpp>

#include <components/esm/globalmap.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/esmstore.hpp"

namespace MWRender
{

    GlobalMap::GlobalMap()
        : mWidth(0)
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
                        if (land && land->mDataTypes & ESM::Land::DATA_WNAM)
                            y = (land->mLandData->mWnam[vertexY * 9 + vertexX] << 4) / 2048.f;
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

    void GlobalMap::exploreCell(int cellX, int cellY)
    {
        //float originX = static_cast<float>((cellX - mMinX) * mCellSize);
        // NB y + 1, because we want the top left corner, not bottom left where the origin of the cell is
        //float originY = static_cast<float>(mHeight - (cellY + 1 - mMinY) * mCellSize);

        if (cellX > mMaxX || cellX < mMinX || cellY > mMaxY || cellY < mMinY)
            return;

        /*
        Ogre::TexturePtr localMapTexture = Ogre::TextureManager::getSingleton().getByName("Cell_"
            + boost::lexical_cast<std::string>(cellX) + "_" + boost::lexical_cast<std::string>(cellY));

        if (!localMapTexture.isNull())
        {
            int mapWidth = localMapTexture->getWidth();
            int mapHeight = localMapTexture->getHeight();
            mOverlayTexture->load();
            mOverlayTexture->getBuffer()->blit(localMapTexture->getBuffer(), Ogre::Image::Box(0,0,mapWidth,mapHeight),
                         Ogre::Image::Box(static_cast<Ogre::uint32>(originX), static_cast<Ogre::uint32>(originY),
                         static_cast<Ogre::uint32>(originX + mCellSize), static_cast<Ogre::uint32>(originY + mCellSize)));

            Ogre::Image backup;
            std::vector<Ogre::uchar> data;
            data.resize(mCellSize*mCellSize*4, 0);
            backup.loadDynamicImage(&data[0], mCellSize, mCellSize, Ogre::PF_A8B8G8R8);

            localMapTexture->getBuffer()->blitToMemory(Ogre::Image::Box(0,0,mapWidth,mapHeight), backup.getPixelBox());

            for (int x=0; x<mCellSize; ++x)
                for (int y=0; y<mCellSize; ++y)
                {
                    assert (originX+x < mOverlayImage.getWidth());
                    assert (originY+y < mOverlayImage.getHeight());
                    assert (x < int(backup.getWidth()));
                    assert (y < int(backup.getHeight()));
                    mOverlayImage.setColourAt(backup.getColourAt(x, y, 0), static_cast<size_t>(originX + x), static_cast<size_t>(originY + y), 0);
                }
        }
        */
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
        mOverlayImage->dirty();

        if (!mOverlayTexture)
        {
            mOverlayTexture = new osg::Texture2D;
            mOverlayTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            mOverlayTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            mOverlayTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
            mOverlayTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
            mOverlayTexture->setImage(mOverlayImage);
            mOverlayTexture->setResizeNonPowerOfTwoHint(false);
        }
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
            std::cerr << "Can't write map overlay: " << result.message() << std::endl;
            return;
        }

        std::string data = ostream.str();
        map.mImageData = std::vector<char>(data.begin(), data.end());
    }

    struct Box
    {
        int mLeft, mRight, mTop, mBottom;

        Box(int left, int right, int top, int bottom)
            : mLeft(left), mRight(right), mTop(top), mBottom(bottom)
        {
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
            std::cerr << "Can't read map overlay: " << result.message() << std::endl;
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

        if (srcBox.mLeft == destBox.mLeft && srcBox.mRight == destBox.mRight
                && srcBox.mTop == destBox.mTop && srcBox.mBottom == destBox.mBottom
                && imageWidth == mWidth && imageHeight == mHeight)
        {
            mOverlayImage->copySubImage(0, 0, 0, image);
        }
        else
        {
            // TODO:
            // Dimensions don't match. This could mean a changed map region, or a changed map resolution.
            // In the latter case, we'll want to use filtering.
            // Create a RTT Camera and draw the image onto mOverlayImage in the next frame?
        }
        mOverlayImage->dirty();
    }

    osg::ref_ptr<osg::Texture2D> GlobalMap::getBaseTexture()
    {
        return mBaseTexture;
    }

    osg::ref_ptr<osg::Texture2D> GlobalMap::getOverlayTexture()
    {
        return mOverlayTexture;
    }
}
