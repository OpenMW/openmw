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
        terrainData.inputFloat = store->land->landData->heights;

        std::map<uint16_t, int> indexes;
        initTerrainTextures(&terrainData, store, indexes);
        mTerrainGroup->defineTerrain(x, y, &terrainData);

        mTerrainGroup->loadTerrain(x, y, true);
        Ogre::Terrain* terrain = mTerrainGroup->getTerrain(x,y);
        initTerrainBlendMaps(terrain, store, indexes);
    }

    void TerrainManager::cellRemoved(MWWorld::Ptr::CellStore *store)
    {
        mTerrainGroup->removeTerrain(store->cell->getGridX(),
                                     store->cell->getGridY());
    }

    void TerrainManager::initTerrainTextures(Ogre::Terrain::ImportData* terrainData,
                                             MWWorld::Ptr::CellStore* store,
                                             std::map<uint16_t, int>& indexes)
    {
        //have a base texture for now, but this is probably not needed on most cells
        terrainData->layerList.resize(1);
        terrainData->layerList[0].worldSize = 256;
        terrainData->layerList[0].textureNames.push_back("textures\\_land_default.dds");
        terrainData->layerList[0].textureNames.push_back("textures\\_land_default.dds");

        const uint16_t* const textures = store->land->landData->textures;
        for ( int y = 0; y < ESM::Land::LAND_TEXTURE_SIZE; y++ )
        {
            for ( int x = 0; x < ESM::Land::LAND_TEXTURE_SIZE; x++ )
            {
                const uint16_t ltexIndex = textures[y * ESM::Land::LAND_TEXTURE_SIZE + x];
                if ( ltexIndex == 0 )
                {
                    continue;
                }

                const std::map<uint16_t, int>::const_iterator it = indexes.find(ltexIndex);

                if ( it == indexes.end() )
                {
                    //NB: All vtex ids are +1 compared to the ltex ids
                    assert((int)ltexIndex - 1 > 0 &&
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
                                              const std::map<uint16_t, int>& indexes)
    {
        const int blendSize = terrain->getLayerBlendMapSize();
        const int blendDist = TERRAIN_SHADE_DISTANCE *
                                  (blendSize / ESM::Land::LAND_TEXTURE_SIZE);

        //zero out every map
        std::map<uint16_t, int>::const_iterator iter;
        for ( iter = indexes.begin(); iter != indexes.end(); ++iter )
        {
             float* pBlend = terrain->getLayerBlendMap(iter->second)
                                    ->getBlendPointer();
             memset(pBlend, 0, sizeof(float) * blendSize * blendSize);
        }

        //covert the ltex data into a set of blend maps
        const uint16_t* const textures = store->land->landData->textures;
        for ( int texY = 0; texY < ESM::Land::LAND_TEXTURE_SIZE; texY++ )
        {
            for ( int texX = 0; texX < ESM::Land::LAND_TEXTURE_SIZE; texX++ )
            {
                const uint16_t ltexIndex = textures[texY * ESM::Land::LAND_TEXTURE_SIZE + texX];
                if ( ltexIndex == 0 ){
                    continue;
                }
                const int layerIndex = indexes.find(ltexIndex)->second;

                float* const pBlend = terrain->getLayerBlendMap(layerIndex)
                                             ->getBlendPointer();

                const int splatSize = blendSize / ESM::Land::LAND_TEXTURE_SIZE;

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

}
