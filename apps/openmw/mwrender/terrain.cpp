#include <OgreTerrain.h>
#include <OgreTerrainGroup.h>

#include "terrain.hpp"

#include "components/esm/loadland.hpp"

namespace MWRender
{
    TerrainManager::TerrainManager(Ogre::SceneManager* mgr)
    {
        mTerrainGlobals = OGRE_NEW Ogre::TerrainGlobalOptions();

        mTerrainGlobals->setMaxPixelError(8);

        mTerrainGroup = OGRE_NEW Ogre::TerrainGroup(mgr,
                Ogre::Terrain::ALIGN_X_Z, ESM::Land::LAND_SIZE,
                ESM::Land::REAL_SIZE);

        mTerrainGroup->setOrigin(Ogre::Vector3(ESM::Land::REAL_SIZE/2,
                                 0,
                                 -ESM::Land::REAL_SIZE/2));

        Ogre::Terrain::ImportData importSettings =
                mTerrainGroup->getDefaultImportSettings();

        importSettings.terrainSize = ESM::Land::LAND_SIZE;
        importSettings.worldSize = ESM::Land::REAL_SIZE;
        importSettings.minBatchSize = 9;
        importSettings.maxBatchSize = 33;

        importSettings.deleteInputData = false;
    }

    TerrainManager::~TerrainManager()
    {
        OGRE_DELETE mTerrainGroup;
        OGRE_DELETE mTerrainGlobals;
    }

    void TerrainManager::cellAdded(MWWorld::Ptr::CellStore *store)
    {
        int x = store->cell->getGridX();
        int y = store->cell->getGridY();

        Ogre::Terrain::ImportData terrainData;

        terrainData.inputBias = 0;
        terrainData.inputFloat = store->land[1][1]->landData->heights;

        std::map<uint16_t, int> indexes;
        initTerrainTextures(&terrainData, store, 0, 0,
                            ESM::Land::LAND_TEXTURE_SIZE, indexes);
        mTerrainGroup->defineTerrain(x, y, &terrainData);

        mTerrainGroup->loadTerrain(x, y, true);
        Ogre::Terrain* terrain = mTerrainGroup->getTerrain(x,y);
        initTerrainBlendMaps(terrain, store, 0, 0,
                             ESM::Land::LAND_TEXTURE_SIZE, indexes);
    }

    void TerrainManager::cellRemoved(MWWorld::Ptr::CellStore *store)
    {
        mTerrainGroup->removeTerrain(store->cell->getGridX(),
                                     store->cell->getGridY());
    }

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

        //have a base texture for now, but this is probably not needed on most cells
        terrainData->layerList.resize(1);
        terrainData->layerList[0].worldSize = 256;
        terrainData->layerList[0].textureNames.push_back("textures\\_land_default.dds");
        terrainData->layerList[0].textureNames.push_back("textures\\_land_default.dds");

        for ( int y = fromY - 1; y < fromY + size + 1; y++ )
        {
            for ( int x = fromX - 1; x < fromX + size + 1; x++ )
            {
                const uint16_t ltexIndex = getLtexIndexAt(store, x, y);
                //this is the base texture, so we can ignore this at present
                if ( ltexIndex == 0 )
                {
                    continue;
                }

                const std::map<uint16_t, int>::const_iterator it = indexes.find(ltexIndex);

                if ( it == indexes.end() )
                {
                    //NB: All vtex ids are +1 compared to the ltex ids
                    assert((int)ltexIndex >= 0 &&
                           store->landTextures->ltex.size() > (size_t)ltexIndex - 1 &&
                           "LAND.VTEX must be within the bounds of the LTEX array");
                    
                    std::string texture = store->landTextures->ltex[ltexIndex-1].texture;
                    //TODO this is needed due to MWs messed up texture handling
                    texture = texture.substr(0, texture.rfind(".")) + ".dds";

                    const size_t position = terrainData->layerList.size();
                    terrainData->layerList.push_back(Ogre::Terrain::LayerInstance());

                    terrainData->layerList[position].worldSize = 256;
                    terrainData->layerList[position].textureNames.push_back("textures\\" + texture);

                    //Normal map. This should be removed but it would require alterations to
                    //the material generator. Another option would be to use a 1x1 blank texture
                    terrainData->layerList[position].textureNames.push_back("textures\\" + texture);

                    indexes[ltexIndex] = position;
                }
            }
        }

    }

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
            for ( int texX = fromY - 1; texX < fromY + size + 1; texX++ )
            {
                const uint16_t ltexIndex = getLtexIndexAt(store, texX, texY);

                //this is the ground texture, which is currently the base texture
                //so don't alter the splatting map
                if ( ltexIndex == 0 ){
                    continue;
                }

                const int layerIndex = indexes.find(ltexIndex)->second;

                float* const pBlend = terrain->getLayerBlendMap(layerIndex)
                                             ->getBlendPointer();

                const int splatSize = blendSize / size;

                //setup the bounds for the shading of this texture splat
                const int startX = std::max(0, texX*splatSize - blendDist);
                const int endX = std::min(blendSize, (texX+1)*splatSize + blendDist);

                const int startY = std::max(0, texY*splatSize - blendDist);
                const int endY = std::min(blendSize, (texY+1)*splatSize + blendDist);

                for ( int blendX = startX; blendX < endX; blendX++ )
                {
                    for ( int blendY = startY; blendY < endY; blendY++ )
                    {
                        assert(blendX >= 0 && blendX < blendSize &&
                               "index must be within texture bounds");

                        assert(blendY >= 0 && blendY < blendSize &&
                               "index must be within texture bounds");

                        //calculate the distance from the edge of the square
                        // to the point we are shading
                        int distX = texX*splatSize - blendX;
                        if ( distX < 0 )
                        {
                            distX = std::max(0, blendX - (texX+1)*splatSize);
                        }

                        int distY = texY*splatSize - blendY;
                        if ( distY < 0 )
                        {
                            distY = std::max(0, blendY - (texY+1)*splatSize);
                        }

                        float blendAmount = blendDist - std::sqrt((float)distX*distX + distY*distY);
                        blendAmount /= blendDist;

                        //this is required as blendDist < sqrt(blendDist*blendDist + blendDist*blendDist)
                        //this means that the corners are slightly the "wrong" shape but totaly smooth 
                        //shading for the edges
                        blendAmount = std::max( (float) 0.0, blendAmount);

                        assert(blendAmount >= 0 && "Blend should never be negative");

                        //flips the y
                        const int index = blendSize*(blendSize - 1 - blendY) + blendX;
                        pBlend[index] += blendAmount;
                        pBlend[index] = std::min((float)1, pBlend[index]);
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

}
