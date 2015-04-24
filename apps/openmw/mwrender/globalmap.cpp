#include "globalmap.hpp"

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include <OgreImage.h>
#include <OgreTextureManager.h>
#include <OgreColourValue.h>
#include <OgreHardwareVertexBuffer.h>
#include <OgreRoot.h>
#include <OgreHardwarePixelBuffer.h>

#include <components/loadinglistener/loadinglistener.hpp>
#include <components/settings/settings.hpp>

#include <components/esm/globalmap.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/esmstore.hpp"

namespace MWRender
{

    GlobalMap::GlobalMap(const std::string &cacheDir)
        : mCacheDir(cacheDir)
        , mMinX(0), mMaxX(0)
        , mMinY(0), mMaxY(0)
        , mWidth(0)
        , mHeight(0)
    {
        mCellSize = Settings::Manager::getInt("global map cell size", "Map");
    }

    GlobalMap::~GlobalMap()
    {
        Ogre::TextureManager::getSingleton().remove(mOverlayTexture->getName());
    }

    void GlobalMap::render (Loading::Listener* loadingListener)
    {
        Ogre::TexturePtr tex;

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

        std::vector<Ogre::uchar> data (mWidth * mHeight * 3);

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

        Ogre::DataStreamPtr stream(new Ogre::MemoryDataStream(&data[0], data.size()));

        tex = Ogre::TextureManager::getSingleton ().createManual ("GlobalMap.png", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            Ogre::TEX_TYPE_2D, mWidth, mHeight, 0, Ogre::PF_B8G8R8, Ogre::TU_STATIC);
        tex->loadRawData(stream, mWidth, mHeight, Ogre::PF_B8G8R8);

        tex->load();

        mOverlayTexture = Ogre::TextureManager::getSingleton().createManual("GlobalMapOverlay", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            Ogre::TEX_TYPE_2D, mWidth, mHeight, 0, Ogre::PF_A8B8G8R8, Ogre::TU_DYNAMIC, this);

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
        float originX = static_cast<float>((cellX - mMinX) * mCellSize);
        // NB y + 1, because we want the top left corner, not bottom left where the origin of the cell is
        float originY = static_cast<float>(mHeight - (cellY + 1 - mMinY) * mCellSize);

        if (cellX > mMaxX || cellX < mMinX || cellY > mMaxY || cellY < mMinY)
            return;

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
    }

    void GlobalMap::clear()
    {
        Ogre::uchar* buffer =  OGRE_ALLOC_T(Ogre::uchar, mWidth * mHeight * 4, Ogre::MEMCATEGORY_GENERAL);
        memset(buffer, 0, mWidth * mHeight * 4);

        mOverlayImage.loadDynamicImage(&buffer[0], mWidth, mHeight, 1, Ogre::PF_A8B8G8R8, true); // pass ownership of buffer to image

        mOverlayTexture->load();
    }

    void GlobalMap::loadResource(Ogre::Resource *resource)
    {
        Ogre::Texture* tex = static_cast<Ogre::Texture*>(resource);
        Ogre::ConstImagePtrList list;
        list.push_back(&mOverlayImage);
        tex->_loadImages(list);
    }

    void GlobalMap::write(ESM::GlobalMap& map)
    {
        map.mBounds.mMinX = mMinX;
        map.mBounds.mMaxX = mMaxX;
        map.mBounds.mMinY = mMinY;
        map.mBounds.mMaxY = mMaxY;

        Ogre::DataStreamPtr encoded = mOverlayImage.encode("png");
        map.mImageData.resize(encoded->size());
        encoded->read(&map.mImageData[0], encoded->size());
    }

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

        Ogre::Image image;
        Ogre::DataStreamPtr stream(new Ogre::MemoryDataStream(&map.mImageData[0], map.mImageData.size()));
        image.load(stream, "png");

        int xLength = (bounds.mMaxX-bounds.mMinX+1);
        int yLength = (bounds.mMaxY-bounds.mMinY+1);

        // Size of one cell in image space
        int cellImageSizeSrc = image.getWidth() / xLength;
        if (int(image.getHeight() / yLength) != cellImageSizeSrc)
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
        Ogre::Image::Box srcBox ( std::max(0, leftDiff * cellImageSizeSrc),
                                  std::max(0, topDiff * cellImageSizeSrc),
                                  std::min(image.getWidth(), image.getWidth() - rightDiff * cellImageSizeSrc),
                                  std::min(image.getHeight(), image.getHeight() - bottomDiff * cellImageSizeSrc));

        Ogre::Image::Box destBox ( std::max(0, -leftDiff * cellImageSizeDst),
                                   std::max(0, -topDiff * cellImageSizeDst),
                                   std::min(mOverlayTexture->getWidth(), mOverlayTexture->getWidth() + rightDiff * cellImageSizeDst),
                                   std::min(mOverlayTexture->getHeight(), mOverlayTexture->getHeight() + bottomDiff * cellImageSizeDst));

        // Looks like there is no interface for blitting from memory with src/dst boxes.
        // So we create a temporary texture for blitting.
        Ogre::TexturePtr tex = Ogre::TextureManager::getSingleton().createManual("@temp",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_2D, image.getWidth(),
                                                                                 image.getHeight(), 0, Ogre::PF_A8B8G8R8);
        tex->loadImage(image);

        mOverlayTexture->load();
        mOverlayTexture->getBuffer()->blit(tex->getBuffer(), srcBox, destBox);

        if (srcBox.left == destBox.left && srcBox.right == destBox.right
                && srcBox.top == destBox.top && srcBox.bottom == destBox.bottom
                && int(image.getWidth()) == mWidth && int(image.getHeight()) == mHeight)
            mOverlayImage = image;
        else
            mOverlayTexture->convertToImage(mOverlayImage);

        Ogre::TextureManager::getSingleton().remove("@temp");
    }
}
