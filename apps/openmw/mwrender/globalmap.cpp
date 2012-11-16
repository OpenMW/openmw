#include "globalmap.hpp"

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include <OgreImage.h>
#include <OgreTextureManager.h>
#include <OgreColourValue.h>
#include <OgreHardwareVertexBuffer.h>
#include <OgreRoot.h>
#include <OgreHardwarePixelBuffer.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/esmstore.hpp"

namespace MWRender
{

    GlobalMap::GlobalMap(const std::string &cacheDir)
        : mCacheDir(cacheDir)
        , mMinX(0), mMaxX(0)
        , mMinY(0), mMaxY(0)
    {
    }


    void GlobalMap::render ()
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

        int cellSize = 24;
        mWidth = cellSize*(mMaxX-mMinX+1);
        mHeight = cellSize*(mMaxY-mMinY+1);

        mExploredBuffer.resize((mMaxX-mMinX+1) * (mMaxY-mMinY+1) * 4);

        //if (!boost::filesystem::exists(mCacheDir + "/GlobalMap.png"))
        if (1)
        {
            Ogre::Image image;

            std::vector<Ogre::uchar> data (mWidth * mHeight * 3);

            for (int x = mMinX; x <= mMaxX; ++x)
            {
                for (int y = mMinY; y <= mMaxY; ++y)
                {
                    ESM::Land* land = esmStore.get<ESM::Land>().search (x,y);

                    if (land)
                    {
                        if (!land->isDataLoaded(ESM::Land::DATA_VHGT))
                        {
                            land->loadData(ESM::Land::DATA_VHGT);
                        }
                    }

                    for (int cellY=0; cellY<cellSize; ++cellY)
                    {
                        for (int cellX=0; cellX<cellSize; ++cellX)
                        {
                            int vertexX = float(cellX)/float(cellSize) * ESM::Land::LAND_SIZE;
                            int vertexY = float(cellY)/float(cellSize) * ESM::Land::LAND_SIZE;


                            int texelX = (x-mMinX) * cellSize + cellX;
                            int texelY = (mHeight-1) - ((y-mMinY) * cellSize + cellY);

                            Ogre::ColourValue waterShallowColour(0.15, 0.2, 0.19);
                            Ogre::ColourValue waterDeepColour(0.1, 0.14, 0.13);
                            Ogre::ColourValue groundColour(0.254, 0.19, 0.13);
                            Ogre::ColourValue mountainColour(0.05, 0.05, 0.05);
                            Ogre::ColourValue hillColour(0.16, 0.12, 0.08);

                            float mountainHeight = 15000.f;
                            float hillHeight = 2500.f;

                            unsigned char r,g,b;

                            if (land)
                            {
                                float landHeight = land->mLandData->mHeights[vertexY * ESM::Land::LAND_SIZE + vertexX];


                                if (landHeight >= 0)
                                {
                                    if (landHeight >= hillHeight)
                                    {
                                        float factor = std::min(1.f, float(landHeight-hillHeight)/mountainHeight);
                                        r = (hillColour.r * (1-factor) + mountainColour.r * factor) * 255;
                                        g = (hillColour.g * (1-factor) + mountainColour.g * factor) * 255;
                                        b = (hillColour.b * (1-factor) + mountainColour.b * factor) * 255;
                                    }
                                    else
                                    {
                                        float factor = std::min(1.f, float(landHeight)/hillHeight);
                                        r = (groundColour.r * (1-factor) + hillColour.r * factor) * 255;
                                        g = (groundColour.g * (1-factor) + hillColour.g * factor) * 255;
                                        b = (groundColour.b * (1-factor) + hillColour.b * factor) * 255;
                                    }
                                }
                                else
                                {
                                    if (landHeight >= -100)
                                    {
                                        float factor = std::min(1.f, -1*landHeight/100.f);
                                        r = (((waterShallowColour+groundColour)/2).r * (1-factor) + waterShallowColour.r * factor) * 255;
                                        g = (((waterShallowColour+groundColour)/2).g * (1-factor) + waterShallowColour.g * factor) * 255;
                                        b = (((waterShallowColour+groundColour)/2).b * (1-factor) + waterShallowColour.b * factor) * 255;
                                    }
                                    else
                                    {
                                        float factor = std::min(1.f, -1*(landHeight-100)/1000.f);
                                        r = (waterShallowColour.r * (1-factor) + waterDeepColour.r * factor) * 255;
                                        g = (waterShallowColour.g * (1-factor) + waterDeepColour.g * factor) * 255;
                                        b = (waterShallowColour.b * (1-factor) + waterDeepColour.b * factor) * 255;
                                    }
                                }

                            }
                            else
                            {
                                r = waterDeepColour.r * 255;
                                g = waterDeepColour.g * 255;
                                b = waterDeepColour.b * 255;
                            }

                            // uncomment this line to outline cell borders
                            //if (cellX == 0 || cellX == cellSize-1 || cellY == 0|| cellY == cellSize-1) r = 255;

                            data[texelY * mWidth * 3 + texelX * 3] = r;
                            data[texelY * mWidth * 3 + texelX * 3+1] = g;
                            data[texelY * mWidth * 3 + texelX * 3+2] = b;
                        }
                    }
                }
            }

            image.loadDynamicImage (&data[0], mWidth, mHeight, Ogre::PF_B8G8R8);

            //image.save (mCacheDir + "/GlobalMap.png");

            tex = Ogre::TextureManager::getSingleton ().createManual ("GlobalMap.png", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                Ogre::TEX_TYPE_2D, mWidth, mHeight, 0, Ogre::PF_B8G8R8, Ogre::TU_STATIC);
            tex->loadImage(image);
        }
        else
            tex = Ogre::TextureManager::getSingleton ().getByName ("GlobalMap.png");

        tex->load();




        mOverlayTexture = Ogre::TextureManager::getSingleton().createManual("GlobalMapOverlay", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            Ogre::TEX_TYPE_2D, mWidth, mHeight, 0, Ogre::PF_A8B8G8R8, Ogre::TU_DYNAMIC_WRITE_ONLY);


        std::vector<Ogre::uint32> buffer;
        buffer.resize(mWidth * mHeight);

        // initialize to (0, 0, 0, 0)
        for (int p=0; p<mWidth * mHeight; ++p)
            buffer[p] = 0;

        memcpy(mOverlayTexture->getBuffer()->lock(Ogre::HardwareBuffer::HBL_DISCARD), &buffer[0], mWidth*mHeight*4);
        mOverlayTexture->getBuffer()->unlock();
    }

    void GlobalMap::worldPosToImageSpace(float x, float z, float& imageX, float& imageY)
    {
        imageX = float(x / 8192.f - mMinX) / (mMaxX - mMinX + 1);

        imageY = 1.f-float(-z / 8192.f - mMinY) / (mMaxY - mMinY + 1);
    }

    void GlobalMap::cellTopLeftCornerToImageSpace(int x, int y, float& imageX, float& imageY)
    {
        imageX = float(x - mMinX) / (mMaxX - mMinX + 1);

        // NB y + 1, because we want the top left corner, not bottom left where the origin of the cell is
        imageY = 1.f-float(y - mMinY + 1) / (mMaxY - mMinY + 1);
    }

    void GlobalMap::exploreCell(int cellX, int cellY)
    {
        float originX = (cellX - mMinX) * 24;
        // NB y + 1, because we want the top left corner, not bottom left where the origin of the cell is
        float originY = mHeight - (cellY+1 - mMinY) * 24;

        if (cellX > mMaxX || cellX < mMinX || cellY > mMaxY || cellY < mMinY)
            return;

        Ogre::TexturePtr localMapTexture = Ogre::TextureManager::getSingleton().getByName("Cell_"
            + boost::lexical_cast<std::string>(cellX) + "_" + boost::lexical_cast<std::string>(cellY));

        // mipmap version - can't get ogre to generate automips..
        /*if (!localMapTexture.isNull())
        {
            assert(localMapTexture->getBuffer(0, 4)->getWidth() == 64); // 1024 / 2^4

            mOverlayTexture->getBuffer()->blit(localMapTexture->getBuffer(0, 4), Ogre::Image::Box(0,0,64, 64),
                         Ogre::Image::Box(originX,originY,originX+24,originY+24));
        }*/

        if (!localMapTexture.isNull())
        {

            mOverlayTexture->getBuffer()->blit(localMapTexture->getBuffer(), Ogre::Image::Box(0,0,512,512),
                         Ogre::Image::Box(originX,originY,originX+24,originY+24));
        }
    }
}
