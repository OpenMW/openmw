#include <OgreTerrain.h>
#include <OgreTerrainGroup.h>

#include "terrainmaterial.hpp"
#include "terrain.hpp"

#include "components/esm/loadland.hpp"

using namespace Ogre;

namespace MWRender
{

    //----------------------------------------------------------------------------------------------
    
    TerrainManager::TerrainManager(Ogre::SceneManager* mgr)
    {
        mTerrainGlobals = OGRE_NEW Ogre::TerrainGlobalOptions();

        Ogre::TerrainMaterialGeneratorPtr matGen;
        TerrainMaterialGeneratorB* matGenP = new TerrainMaterialGeneratorB();
        matGen.bind(matGenP);
        mTerrainGlobals->setDefaultMaterialGenerator(matGen);

        Ogre::TerrainMaterialGenerator::Profile* const activeProfile =
            mTerrainGlobals->getDefaultMaterialGenerator()
                           ->getActiveProfile();
        TerrainMaterialGeneratorB::SM2Profile* matProfile =
            static_cast<TerrainMaterialGeneratorB::SM2Profile*>(activeProfile);

        mTerrainGlobals->setMaxPixelError(8);
        mTerrainGlobals->setLayerBlendMapSize(SPLIT_TERRAIN ? 256 : 1024);
        mTerrainGlobals->setLightMapSize(SPLIT_TERRAIN ? 256 : 1024);
        mTerrainGlobals->setCompositeMapSize(SPLIT_TERRAIN ? 256 : 1024);
        mTerrainGlobals->setDefaultGlobalColourMapSize(256);

        //10 (default) didn't seem to be quite enough
        mTerrainGlobals->setSkirtSize(128);

        /*
         * Here we are pushing the composite map distance beyond the edge
         * of the rendered terrain due to not having setup lighting
         */
        //mTerrainGlobals->setCompositeMapDistance(ESM::Land::REAL_SIZE*4);
        
        matProfile->setLightmapEnabled(false);
        matProfile->setLayerSpecularMappingEnabled(false);
        matProfile->setLayerNormalMappingEnabled(false);
        matProfile->setLayerParallaxMappingEnabled(false);
        matProfile->setReceiveDynamicShadowsEnabled(false);

        mLandSize = ESM::Land::LAND_SIZE;
        mRealSize = ESM::Land::REAL_SIZE;
        if ( SPLIT_TERRAIN )
        {
            mLandSize  = (mLandSize - 1)/2 + 1;
            mRealSize /= 2;
        }

        mTerrainGroup = OGRE_NEW Ogre::TerrainGroup(mgr,
                                                    Ogre::Terrain::ALIGN_X_Z,
                                                    mLandSize,
                                                    mRealSize);

        mTerrainGroup->setOrigin(Ogre::Vector3(mRealSize/2,
                                               0,
                                               -mRealSize/2));

        Ogre::Terrain::ImportData& importSettings =
                mTerrainGroup->getDefaultImportSettings();

        importSettings.inputBias    = 0;
        importSettings.terrainSize  = mLandSize;
        importSettings.worldSize    = mRealSize;
        importSettings.minBatchSize = 9;
        importSettings.maxBatchSize = mLandSize;

        importSettings.deleteInputData = true;
    }

    //----------------------------------------------------------------------------------------------

    TerrainManager::~TerrainManager()
    {
        OGRE_DELETE mTerrainGroup;
        OGRE_DELETE mTerrainGlobals;
    }
    
    //----------------------------------------------------------------------------------------------
    
    void TerrainManager::setDiffuse(const Ogre::ColourValue& diffuse)
    {
        mTerrainGlobals->setCompositeMapDiffuse(diffuse);
    }
    
    //----------------------------------------------------------------------------------------------
    
    void TerrainManager::setAmbient(const Ogre::ColourValue& ambient)
    {
        mTerrainGlobals->setCompositeMapAmbient(ambient);
    }

    //----------------------------------------------------------------------------------------------

    void TerrainManager::cellAdded(MWWorld::Ptr::CellStore *store)
    {
        const int cellX = store->cell->getGridX();
        const int cellY = store->cell->getGridY();


        if ( SPLIT_TERRAIN )
        {
            //split the cell terrain into four segments
            const int numTextures = ESM::Land::LAND_TEXTURE_SIZE/2;

            for ( int x = 0; x < 2; x++ )
            {
                for ( int y = 0; y < 2; y++ )
                {
                    Ogre::Terrain::ImportData terrainData =
                        mTerrainGroup->getDefaultImportSettings();

                    const int terrainX = cellX * 2 + x;
                    const int terrainY = cellY * 2 + y;

                    terrainData.inputFloat = OGRE_ALLOC_T(float,
                                                          mLandSize*mLandSize,
                                                          Ogre::MEMCATEGORY_GEOMETRY);

                    //copy the height data row by row
                    for ( int terrainCopyY = 0; terrainCopyY < mLandSize; terrainCopyY++ )
                    {
                                               //the offset of the current segment
                        const size_t yOffset = y * (mLandSize-1) * ESM::Land::LAND_SIZE +
                                               //offset of the row
                                               terrainCopyY * ESM::Land::LAND_SIZE;
                        const size_t xOffset = x * (mLandSize-1);

                        memcpy(&terrainData.inputFloat[terrainCopyY*mLandSize],
                               &store->land[1][1]->landData->heights[yOffset + xOffset],
                               mLandSize*sizeof(float));
                    }

                    std::map<uint16_t, int> indexes;
                    initTerrainTextures(&terrainData, store,
                                        x * numTextures, y * numTextures,
                                        numTextures, indexes);

                    if (mTerrainGroup->getTerrain(cellX, cellY) == NULL)
                    {
                        mTerrainGroup->defineTerrain(terrainX, terrainY, &terrainData);

                        mTerrainGroup->loadTerrain(terrainX, terrainY, true);
                        Ogre::Terrain* terrain = mTerrainGroup->getTerrain(terrainX, terrainY);
                        initTerrainBlendMaps(terrain, store,
                                             x * numTextures, y * numTextures,
                                             numTextures, indexes);
                    }
                }
            }
        }
        else
        {
            Ogre::Terrain::ImportData terrainData =
                mTerrainGroup->getDefaultImportSettings();

            //one cell is one terrain segment
            terrainData.inputFloat = OGRE_ALLOC_T(float,
                                                  mLandSize*mLandSize,
                                                  Ogre::MEMCATEGORY_GEOMETRY);

            memcpy(&terrainData.inputFloat[0],
                   &store->land[1][1]->landData->heights[0],
                   mLandSize*mLandSize*sizeof(float));

            std::map<uint16_t, int> indexes;
            initTerrainTextures(&terrainData, store, 0, 0,
                                ESM::Land::LAND_TEXTURE_SIZE, indexes);

            mTerrainGroup->defineTerrain(cellX, cellY, &terrainData);

            mTerrainGroup->loadTerrain(cellX, cellY, true);
            Ogre::Terrain* terrain = mTerrainGroup->getTerrain(cellX, cellY);
            initTerrainBlendMaps(terrain, store, 0, 0,
                                 ESM::Land::LAND_TEXTURE_SIZE, indexes);
        }

        mTerrainGroup->freeTemporaryResources();
    }

    //----------------------------------------------------------------------------------------------

    void TerrainManager::cellRemoved(MWWorld::Ptr::CellStore *store)
    {
        if ( SPLIT_TERRAIN )
        {
            for ( int x = 0; x < 2; x++ )
            {
                for ( int y = 0; y < 2; y++ )
                {
                    mTerrainGroup->unloadTerrain(store->cell->getGridX() * 2 + x,
                                                 store->cell->getGridY() * 2 + y);
                }
            }
        }
        else
        {
            mTerrainGroup->unloadTerrain(store->cell->getGridX(),
                                         store->cell->getGridY());
        }
    }

    //----------------------------------------------------------------------------------------------

    void TerrainManager::initTerrainTextures(Ogre::Terrain::ImportData* terrainData,
                                             MWWorld::Ptr::CellStore* store,
                                             int fromX, int fromY, int size,
                                             std::map<uint16_t, int>& indexes)
    {
        assert(store != NULL && "store must be a valid pointer");
        assert(terrainData != NULL && "Must have valid terrain data");
        assert(fromX >= 0 && fromY >= 0 &&
               "Can't get a terrain texture on terrain outside the current cell");
        assert(fromX+size <= ESM::Land::LAND_TEXTURE_SIZE &&
               fromY+size <= ESM::Land::LAND_TEXTURE_SIZE &&
               "Can't get a terrain texture on terrain outside the current cell");

        //there is one texture that we want to use as a base (i.e. it won't have
        //a blend map). This holds the ltex index of that base texture so that
        //we know not to include it in the output map
        int baseTexture = -1;
        for ( int y = fromY - 1; y < fromY + size + 1; y++ )
        {
            for ( int x = fromX - 1; x < fromX + size + 1; x++ )
            {
                const uint16_t ltexIndex = getLtexIndexAt(store, x, y);
                //this is the base texture, so we can ignore this at present
                if ( ltexIndex == baseTexture )
                {
                    continue;
                }

                const std::map<uint16_t, int>::const_iterator it = indexes.find(ltexIndex);

                if ( it == indexes.end() )
                {
                    //NB: All vtex ids are +1 compared to the ltex ids
                    assert((int)ltexIndex >= 0 &&
                           (int)store->landTextures->ltex.size() > (int)ltexIndex - 1 &&
                           "LAND.VTEX must be within the bounds of the LTEX array");
                    
                    std::string texture;
                    if ( ltexIndex == 0 )
                    {
                        texture = "_land_default.dds";
                    }
                    else
                    {
                        texture = store->landTextures->ltex[ltexIndex-1].texture;
                        //TODO this is needed due to MWs messed up texture handling
                        texture = texture.substr(0, texture.rfind(".")) + ".dds";
                    }

                    const size_t position = terrainData->layerList.size();
                    terrainData->layerList.push_back(Ogre::Terrain::LayerInstance());

                    Ogre::TexturePtr normDisp = getNormalDisp("textures\\" + texture);

                    terrainData->layerList[position].worldSize = 256;
                    terrainData->layerList[position].textureNames.push_back("textures\\" + texture);

                    //Normal map. This should be removed but it would require alterations to
                    //the material generator. Another option would be to use a 1x1 blank texture
                    //terrainData->layerList[position].textureNames.push_back(normDisp->getName());

                    if ( baseTexture == -1 )
                    {
                        baseTexture = ltexIndex;
                    }
                    else
                    {
                        indexes[ltexIndex] = position;
                    }
                }
            }
        }
    }

    //----------------------------------------------------------------------------------------------

    void TerrainManager::initTerrainBlendMaps(Ogre::Terrain* terrain,
                                              MWWorld::Ptr::CellStore* store,
                                              int fromX, int fromY, int size,
                                              const std::map<uint16_t, int>& indexes)
    {
        assert(store != NULL && "store must be a valid pointer");
        assert(terrain != NULL && "Must have valid terrain");
        assert(fromX >= 0 && fromY >= 0 &&
               "Can't get a terrain texture on terrain outside the current cell");
        assert(fromX+size <= ESM::Land::LAND_TEXTURE_SIZE &&
               fromY+size <= ESM::Land::LAND_TEXTURE_SIZE &&
               "Can't get a terrain texture on terrain outside the current cell");

        //size must be a power of 2 as we do divisions with a power of 2 number
        //that need to result in an integer for correct splatting
        assert( (size & (size - 1)) == 0 && "Size must be a power of 2");

        const int blendSize = terrain->getLayerBlendMapSize();
        const int blendDist = TERRAIN_SHADE_DISTANCE * (blendSize / size);

        //zero out every map
        std::map<uint16_t, int>::const_iterator iter;
        for ( iter = indexes.begin(); iter != indexes.end(); ++iter )
        {
            float* pBlend = terrain->getLayerBlendMap(iter->second)
                                   ->getBlendPointer();
            memset(pBlend, 0, sizeof(float) * blendSize * blendSize);
        }

        //covert the ltex data into a set of blend maps
        for ( int texY = fromY - 1; texY < fromY + size + 1; texY++ )
        {
            for ( int texX = fromX - 1; texX < fromX + size + 1; texX++ )
            {
                const uint16_t ltexIndex = getLtexIndexAt(store, texX, texY);

                //whilte texX is the splat index relative to the entire cell,
                //relX is relative to the current segment we are splatting
                const int relX = texX - fromX;
                const int relY = texY - fromY;

                //check if it is the base texture (which isn't in the map) and
                //if it is don't bother altering the blend map for it
                if ( indexes.find(ltexIndex) == indexes.end() )
                {
                    continue;
                }

                const int layerIndex = indexes.find(ltexIndex)->second;

                float* const pBlend = terrain->getLayerBlendMap(layerIndex)
                                             ->getBlendPointer();

                const int splatSize = blendSize / size;

                //setup the bounds for the shading of this texture splat
                const int startX = std::max(0, relX*splatSize - blendDist);
                const int endX = std::min(blendSize, (relX+1)*splatSize + blendDist);

                const int startY = std::max(0, relY*splatSize - blendDist);
                const int endY = std::min(blendSize, (relY+1)*splatSize + blendDist);
                
                for ( int blendX = startX; blendX < endX; blendX++ )
                {
                    for ( int blendY = startY; blendY < endY; blendY++ )
                    {
                        assert(blendX >= 0 && blendX < blendSize &&
                               "index must be within texture bounds");

                        assert(blendY >= 0 && blendY < blendSize &&
                               "index must be within texture bounds");

                        const int index = blendSize*(blendSize - 1 - blendY) + blendX;
                        if ( blendX >= relX*splatSize && blendX < (relX+1)*splatSize &&
                             blendY >= relY*splatSize && blendY < (relY+1)*splatSize )
                        {
                            pBlend[index] = 1;
                        }
                        else
                        {
                            pBlend[index] = std::max((float)pBlend[index], 0.5f);
                        }
                    }
                }
                
            }
        }

        //update the maps
        for ( iter = indexes.begin(); iter != indexes.end(); ++iter )
        {
             Ogre::TerrainLayerBlendMap* blend = terrain->getLayerBlendMap(iter->second);
             blend->dirty();
             blend->update();
        }

    }

    //----------------------------------------------------------------------------------------------

    int TerrainManager::getLtexIndexAt(MWWorld::Ptr::CellStore* store,
                                       int x, int y)
    {
        //check texture index falls within the 9 cell bounds
        //as this function can't cope with anything above that
        assert(x >= -ESM::Land::LAND_TEXTURE_SIZE &&
               y >= -ESM::Land::LAND_TEXTURE_SIZE &&
               "Trying to get land textures that are out of bounds");

        assert(x < 2*ESM::Land::LAND_TEXTURE_SIZE &&
               y < 2*ESM::Land::LAND_TEXTURE_SIZE &&
               "Trying to get land textures that are out of bounds");

        assert(store != NULL && "Store pointer must be valid");

        //default center cell is indexed at (1,1)
        int cellX = 1;
        int cellY = 1;

        if ( x < 0 )
        {
            cellX--;
            x += ESM::Land::LAND_TEXTURE_SIZE;
        }
        else if ( x >= ESM::Land::LAND_TEXTURE_SIZE )
        {
            cellX++;
            x -= ESM::Land::LAND_TEXTURE_SIZE;
        }

        if ( y < 0 )
        {
            cellY--;
            y += ESM::Land::LAND_TEXTURE_SIZE;
        }
        else if ( y >= ESM::Land::LAND_TEXTURE_SIZE )
        {
            cellY++;
            y -= ESM::Land::LAND_TEXTURE_SIZE;
        }

        return store->land[cellX][cellY]
                    ->landData
                    ->textures[y * ESM::Land::LAND_TEXTURE_SIZE + x];
    }

    //----------------------------------------------------------------------------------------------
    
    Ogre::TexturePtr TerrainManager::getNormalDisp(const std::string& fileName)
    {
        Ogre::TextureManager* const texMgr = Ogre::TextureManager::getSingletonPtr();
        const std::string normalTextureName = fileName.substr(0, fileName.rfind("."))
                                       + "_n.dds";
        if ( !texMgr->getByName(normalTextureName).isNull() )
        {
            return texMgr->getByName(normalTextureName);
        }

        const std::string textureName = "default_terrain_normal";
        if ( !texMgr->getByName(textureName).isNull() )
        {
            return texMgr->getByName(textureName);
        }

        Ogre::TexturePtr tex = texMgr->createManual(
                 textureName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                 Ogre::TEX_TYPE_2D, 1, 1, 0, Ogre::PF_BYTE_BGRA);

        Ogre::HardwarePixelBufferSharedPtr pixelBuffer = tex->getBuffer();
         
        pixelBuffer->lock(Ogre::HardwareBuffer::HBL_NORMAL);
        const Ogre::PixelBox& pixelBox = pixelBuffer->getCurrentLock();
         
        Ogre::uint8* pDest = static_cast<Ogre::uint8*>(pixelBox.data);
         
        *pDest++ = 128; // B
        *pDest++ = 128; // G
        *pDest++ = 128; // R
        *pDest++ = 0;   // A
         
        pixelBuffer->unlock();

        return tex;
    }

}
