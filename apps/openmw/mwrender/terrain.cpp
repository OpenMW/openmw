#include <OgreTerrain.h>
#include <OgreTerrainGroup.h>

#include "terrainmaterial.hpp"
#include "terrain.hpp"

#include "components/esm/loadland.hpp"

#include <boost/lexical_cast.hpp>

using namespace Ogre;

namespace MWRender
{

    //----------------------------------------------------------------------------------------------
    
    TerrainManager::TerrainManager(SceneManager* mgr)
    {
        mTerrainGlobals = OGRE_NEW TerrainGlobalOptions();

        TerrainMaterialGeneratorPtr matGen;
        TerrainMaterialGeneratorB* matGenP = new TerrainMaterialGeneratorB();
        matGen.bind(matGenP);
        mTerrainGlobals->setDefaultMaterialGenerator(matGen);

        TerrainMaterialGenerator::Profile* const activeProfile =
            mTerrainGlobals->getDefaultMaterialGenerator()
                           ->getActiveProfile();
        TerrainMaterialGeneratorB::SM2Profile* matProfile =
            static_cast<TerrainMaterialGeneratorB::SM2Profile*>(activeProfile);

        //The pixel error should be as high as possible without it being noticed
        //as it governs how fast mesh quality decreases. 16 was just about Ok
        //when tested at the small swamp pond in Seyda Neen
        mTerrainGlobals->setMaxPixelError(16);

        mTerrainGlobals->setLayerBlendMapSize(32);
        mTerrainGlobals->setLightMapSize(256);
        mTerrainGlobals->setCompositeMapSize(256);
        mTerrainGlobals->setDefaultGlobalColourMapSize(256);

        //10 (default) didn't seem to be quite enough
        mTerrainGlobals->setSkirtSize(128);

        //due to the sudden flick between composite and non composite textures,
        //this seemed the distance where it wasn't too noticeable
        mTerrainGlobals->setCompositeMapDistance(mWorldSize*2);
        
        matProfile->setLightmapEnabled(false);
        matProfile->setLayerSpecularMappingEnabled(false);
        matProfile->setLayerNormalMappingEnabled(false);
        matProfile->setLayerParallaxMappingEnabled(false);
        matProfile->setReceiveDynamicShadowsEnabled(false);
        matProfile->setGlobalColourMapEnabled(true);

        mTerrainGroup = OGRE_NEW TerrainGroup(mgr,
                                                    Terrain::ALIGN_X_Z,
                                                    mLandSize,
                                                    mWorldSize);

        mTerrainGroup->setOrigin(Vector3(mWorldSize/2,
                                               0,
                                               -mWorldSize/2));

        Terrain::ImportData& importSettings =
                mTerrainGroup->getDefaultImportSettings();

        importSettings.inputBias    = 0;
        importSettings.terrainSize  = mLandSize;
        importSettings.worldSize    = mWorldSize;
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
    
    void TerrainManager::setDiffuse(const ColourValue& diffuse)
    {
        mTerrainGlobals->setCompositeMapDiffuse(diffuse);
    }
    
    //----------------------------------------------------------------------------------------------
    
    void TerrainManager::setAmbient(const ColourValue& ambient)
    {
        mTerrainGlobals->setCompositeMapAmbient(ambient);
    }

    //----------------------------------------------------------------------------------------------

    void TerrainManager::cellAdded(MWWorld::Ptr::CellStore *store)
    {
        const int cellX = store->cell->getGridX();
        const int cellY = store->cell->getGridY();


        //split the cell terrain into four segments
        const int numTextures = ESM::Land::LAND_TEXTURE_SIZE/2;

        for ( int x = 0; x < 2; x++ )
        {
            for ( int y = 0; y < 2; y++ )
            {
                Terrain::ImportData terrainData =
                    mTerrainGroup->getDefaultImportSettings();

                const int terrainX = cellX * 2 + x;
                const int terrainY = cellY * 2 + y;

                //it makes far more sense to reallocate the memory here,
                //and let Ogre deal with it due to the issues with deleting
                //it at the wrong time if using threads (Which Terrain does)
                terrainData.inputFloat = OGRE_ALLOC_T(float,
                                                      mLandSize*mLandSize,
                                                      MEMCATEGORY_GEOMETRY);

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

                if (mTerrainGroup->getTerrain(terrainX, terrainY) == NULL)
                {
                    mTerrainGroup->defineTerrain(terrainX, terrainY, &terrainData);

                    mTerrainGroup->loadTerrain(terrainX, terrainY, true);

                    Terrain* terrain = mTerrainGroup->getTerrain(terrainX, terrainY);
                    initTerrainBlendMaps(terrain, store,
                                         x * numTextures, y * numTextures,
                                         numTextures,
                                         indexes);

                    if ( store->land[1][1]->landData->usingColours )
                    {
                        TexturePtr vertex = getVertexColours(store,
                                                                   x*(mLandSize-1),
                                                                   y*(mLandSize-1),
                                                                   mLandSize);

                        //this is a hack to get around the fact that Ogre seems to
                        //corrupt the composite map leading to rendering errors
                        MaterialPtr mat = terrain->_getMaterial();
                        mat->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName( vertex->getName() );
                        mat = terrain->_getCompositeMapMaterial();
                        mat->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName( vertex->getName() );
                    }
                }
            }
        }

        mTerrainGroup->freeTemporaryResources();
    }

    //----------------------------------------------------------------------------------------------

    void TerrainManager::cellRemoved(MWWorld::Ptr::CellStore *store)
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

    //----------------------------------------------------------------------------------------------

    void TerrainManager::initTerrainTextures(Terrain::ImportData* terrainData,
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

        //this ensures that the ltex indexes are sorted (or retrived as sorted
        //which simplifies shading between cells).
        //
        //If we don't sort the ltex indexes, the splatting order may differ between
        //cells which may lead to inconsistent results when shading between cells
        std::set<uint16_t> ltexIndexes;
        for ( int y = fromY - 1; y < fromY + size + 1; y++ )
        {
            for ( int x = fromX - 1; x < fromX + size + 1; x++ )
            {
                ltexIndexes.insert(getLtexIndexAt(store, x, y));
            }
        }

        //there is one texture that we want to use as a base (i.e. it won't have
        //a blend map). This holds the ltex index of that base texture so that
        //we know not to include it in the output map
        int baseTexture = -1;
        for ( std::set<uint16_t>::iterator iter = ltexIndexes.begin();
              iter != ltexIndexes.end();
              ++iter )
        {
            const uint16_t ltexIndex = *iter;
            //this is the base texture, so we can ignore this at present
            if ( ltexIndex == baseTexture )
            {
                continue;
            }

            const std::map<uint16_t, int>::const_iterator it = indexes.find(ltexIndex);

            if ( it == indexes.end() )
            {
                //NB: All vtex ids are +1 compared to the ltex ids

                assert( (int)store->landTextures->ltex.size() >= (int)ltexIndex - 1 &&
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
                terrainData->layerList.push_back(Terrain::LayerInstance());

                terrainData->layerList[position].worldSize = 256;
                terrainData->layerList[position].textureNames.push_back("textures\\" + texture);

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

    //----------------------------------------------------------------------------------------------

    void TerrainManager::initTerrainBlendMaps(Terrain* terrain,
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

        const int blendMapSize = terrain->getLayerBlendMapSize();
        const int splatSize    = blendMapSize / size;

        //zero out every map
        std::map<uint16_t, int>::const_iterator iter;
        for ( iter = indexes.begin(); iter != indexes.end(); ++iter )
        {
            float* pBlend = terrain->getLayerBlendMap(iter->second)
                                   ->getBlendPointer();
            memset(pBlend, 0, sizeof(float) * blendMapSize * blendMapSize);
        }

        //covert the ltex data into a set of blend maps
        for ( int texY = fromY - 1; texY < fromY + size + 1; texY++ )
        {
            for ( int texX = fromX - 1; texX < fromX + size + 1; texX++ )
            {
                const uint16_t ltexIndex = getLtexIndexAt(store, texX, texY);

                //check if it is the base texture (which isn't in the map) and
                //if it is don't bother altering the blend map for it
                if ( indexes.find(ltexIndex) == indexes.end() )
                {
                    continue;
                }

                //while texX is the splat index relative to the entire cell,
                //relX is relative to the current segment we are splatting
                const int relX = texX - fromX;
                const int relY = texY - fromY;

                const int layerIndex = indexes.find(ltexIndex)->second;

                float* const pBlend = terrain->getLayerBlendMap(layerIndex)
                                             ->getBlendPointer();

                for ( int y = -1; y < splatSize + 1; y++ ){
                    for ( int x = -1; x < splatSize + 1; x++ ){

                        //Note: Y is reversed
                        const int splatY = blendMapSize - 1 - relY * splatSize - y;
                        const int splatX = relX * splatSize + x;

                        if ( splatX >= 0 && splatX < blendMapSize &&
                             splatY >= 0 && splatY < blendMapSize )
                        {
                            const int index = (splatY)*blendMapSize + splatX;

                            if ( y >= 0 && y < splatSize &&
                                 x >= 0 && x < splatSize )
                            {
                                pBlend[index] = 1;
                            }
                            else
                            {
                                //this provides a transition shading but also 
                                //rounds off the corners slightly
                                pBlend[index] = std::min(1.0f, pBlend[index] + 0.5f);
                            }
                        }

                    }
                }
            }
        }

        for ( int i = 1; i < terrain->getLayerCount(); i++ )
        {
             TerrainLayerBlendMap* blend = terrain->getLayerBlendMap(i);
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

    TexturePtr TerrainManager::getVertexColours(MWWorld::Ptr::CellStore* store,
                                                    int fromX, int fromY, int size)
    {
        TextureManager* const texMgr = TextureManager::getSingletonPtr();

        const std::string colourTextureName = "VtexColours_" +
                                              boost::lexical_cast<std::string>(store->cell->getGridX()) +
                                              "_" +
                                              boost::lexical_cast<std::string>(store->cell->getGridY()) +
                                              "_" +
                                              boost::lexical_cast<std::string>(fromX) +
                                              "_" +
                                              boost::lexical_cast<std::string>(fromY);

        TexturePtr tex = texMgr->getByName(colourTextureName);
        if ( !tex.isNull() )
        {
            return tex;
        }

        tex = texMgr->createManual(colourTextureName,
                                   ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                   TEX_TYPE_2D, size, size, 0, PF_BYTE_BGR);

        HardwarePixelBufferSharedPtr pixelBuffer = tex->getBuffer();
         
        pixelBuffer->lock(HardwareBuffer::HBL_DISCARD);
        const PixelBox& pixelBox = pixelBuffer->getCurrentLock();
         
        uint8* pDest = static_cast<uint8*>(pixelBox.data);
         
        const char* const colours = store->land[1][1]->landData->colours;
        for ( int y = 0; y < size; y++ )
        {
            for ( int x = 0; x < size; x++ )
            {
                const size_t colourOffset = (y+fromY)*3*65 + (x+fromX)*3;

                assert( colourOffset >= 0 && colourOffset < 65*65*3 &&
                        "Colour offset is out of the expected bounds of record" );

                const unsigned char r = colours[colourOffset + 0];
                const unsigned char g = colours[colourOffset + 1];
                const unsigned char b = colours[colourOffset + 2];

                //as is the case elsewhere we need to flip the y
                const size_t imageOffset = (size - 1 - y)*size*4 + x*4;
                pDest[imageOffset + 0] = b;
                pDest[imageOffset + 1] = g;
                pDest[imageOffset + 2] = r;
            }
        }
         
        pixelBuffer->unlock();

        return tex;
    }

}
