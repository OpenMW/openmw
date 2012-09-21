#include "globalmap.hpp"

#include <boost/filesystem.hpp>

#include <OgreImage.h>
#include <OgreTextureManager.h>
#include <OgreColourValue.h>
#include <OgreHardwareVertexBuffer.h>
#include <OgreRoot.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include <components/esm_store/store.hpp>
#include <components/esm_store/reclists.hpp>

namespace MWRender
{

    GlobalMap::GlobalMap(const std::string &cacheDir)
        : mCacheDir(cacheDir)
    {
    }


    void GlobalMap::render ()
    {
        Ogre::TexturePtr tex;

        if (!boost::filesystem::exists(mCacheDir + "/GlobalMap.png"))
        {

            int cellSize = 24;

            Ogre::Image image;

            int width = cellSize*61;
            int height = cellSize*61;

            Ogre::uchar data[width * height * 3];

            for (int x = -30; x <= 30; ++x)
            {
                for (int y = -30; y <= 30; ++y)
                {
                    ESM::Land* land = MWBase::Environment::get().getWorld ()->getStore ().lands.search (x,y);

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


                            int texelX = (x+30) * cellSize + cellX;
                            int texelY = (height-1) - ((y+30) * cellSize + cellY);

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

                            data[texelY * height * 3 + texelX * 3] = r;
                            data[texelY * height * 3 + texelX * 3+1] = g;
                            data[texelY * height * 3 + texelX * 3+2] = b;
                        }
                    }
                }
            }

            image.loadDynamicImage (data, width, height, Ogre::PF_B8G8R8);

            image.save (mCacheDir + "/GlobalMap.png");

            tex = Ogre::TextureManager::getSingleton ().createManual ("GlobalMap.png", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                Ogre::TEX_TYPE_2D, width, height, 0, Ogre::PF_B8G8R8, Ogre::TU_DEFAULT);
            tex->loadImage(image);
        }
        else
            tex = Ogre::TextureManager::getSingleton ().getByName ("GlobalMap.png");

        tex->load();
    }

}
