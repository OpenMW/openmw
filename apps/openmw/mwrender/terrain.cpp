#include <boost/lexical_cast.hpp>

#include <OgreTerrain.h>
#include <OgreTerrainGroup.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreRoot.h>

#include "../mwworld/esmstore.hpp"

#include <components/settings/settings.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "terrainmaterial.hpp"
#include "terrain.hpp"
#include "renderconst.hpp"
#include "shadows.hpp"
#include "renderingmanager.hpp"

using namespace Ogre;

namespace MWRender
{

    //----------------------------------------------------------------------------------------------

    TerrainManager::TerrainManager(Ogre::SceneManager* mgr, RenderingManager* rend) :
         mTerrainGroup(TerrainGroup(mgr, Terrain::ALIGN_X_Y, mLandSize, mWorldSize)), mRendering(rend)
    {
        mTerrainGlobals = OGRE_NEW TerrainGlobalOptions();

        TerrainMaterialGeneratorPtr matGen;
        TerrainMaterial* matGenP = new TerrainMaterial();
        matGen.bind(matGenP);
        mTerrainGlobals->setDefaultMaterialGenerator(matGen);

        TerrainMaterialGenerator::Profile* const activeProfile =
            mTerrainGlobals->getDefaultMaterialGenerator()
                           ->getActiveProfile();
        mActiveProfile = static_cast<TerrainMaterial::Profile*>(activeProfile);

        // We don't want any pixel error at all. Really, LOD makes no sense here - morrowind uses 65x65 verts in one cell,
        // so applying LOD is most certainly slower than doing no LOD at all.
        // Setting this to 0 seems to cause glitches though. :/
        mTerrainGlobals->setMaxPixelError(1);

        mTerrainGlobals->setLayerBlendMapSize(32);

        //10 (default) didn't seem to be quite enough
        mTerrainGlobals->setSkirtSize(128);

        //due to the sudden flick between composite and non composite textures,
        //this seemed the distance where it wasn't too noticeable
        mTerrainGlobals->setCompositeMapDistance(mWorldSize*2);

        mTerrainGroup.setOrigin(Vector3(mWorldSize/2,
                                         mWorldSize/2,
                                         0));

        Terrain::ImportData& importSettings = mTerrainGroup.getDefaultImportSettings();

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
        const int cellX = store->mCell->getGridX();
        const int cellY = store->mCell->getGridY();

        ESM::Land* land =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Land>().search(cellX, cellY);
        if (land == NULL) // no land data means we're not going to create any terrain.
            return;

        int dataRequired = ESM::Land::DATA_VHGT | ESM::Land::DATA_VCLR;
        if (!land->isDataLoaded(dataRequired))
        {
            land->loadData(dataRequired);
        }

        //split the cell terrain into four segments
        const int numTextures = ESM::Land::LAND_TEXTURE_SIZE/2;

        for ( int x = 0; x < 2; x++ )
        {
            for ( int y = 0; y < 2; y++ )
            {
                Terrain::ImportData terrainData =
                    mTerrainGroup.getDefaultImportSettings();

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
                           &land->mLandData->mHeights[yOffset + xOffset],
                           mLandSize*sizeof(float));
                }

                std::map<uint16_t, int> indexes;
                initTerrainTextures(&terrainData, cellX, cellY,
                                    x * numTextures, y * numTextures,
                                    numTextures, indexes, land->mPlugin);

                if (mTerrainGroup.getTerrain(terrainX, terrainY) == NULL)
                {
                    mTerrainGroup.defineTerrain(terrainX, terrainY, &terrainData);

                    mTerrainGroup.loadTerrain(terrainX, terrainY, true);

                    Terrain* terrain = mTerrainGroup.getTerrain(terrainX, terrainY);
                    initTerrainBlendMaps(terrain,
                                         cellX, cellY,
                                         x * numTextures, y * numTextures,
                                         numTextures,
                                         indexes);
                    terrain->setVisibilityFlags(RV_Terrain);
                    terrain->setRenderQueueGroup(RQG_Main);

                    // disable or enable global colour map (depends on available vertex colours)
                    if ( land->mLandData->mUsingColours )
                    {
                        TexturePtr vertex = getVertexColours(land,
                                                             cellX, cellY,
                                                             x*(mLandSize-1),
                                                             y*(mLandSize-1),
                                                             mLandSize);

                        mActiveProfile->setGlobalColourMapEnabled(true);
                        mActiveProfile->setGlobalColourMap (terrain, vertex->getName());
                    }
                    else
                        mActiveProfile->setGlobalColourMapEnabled (false);
                }
            }
        }

        // when loading from a heightmap, Ogre::Terrain does not update the derived data (normal map, LOD)
        // synchronously, even if we supply synchronous = true parameter to loadTerrain.
        // the following to be the only way to make sure derived data is ready when rendering the next frame.
        while (mTerrainGroup.isDerivedDataUpdateInProgress())
        {
           // we need to wait for this to finish
           OGRE_THREAD_SLEEP(5);
           Root::getSingleton().getWorkQueue()->processResponses();
        }

        mTerrainGroup.freeTemporaryResources();
    }

    //----------------------------------------------------------------------------------------------

    void TerrainManager::cellRemoved(MWWorld::Ptr::CellStore *store)
    {
        for ( int x = 0; x < 2; x++ )
        {
            for ( int y = 0; y < 2; y++ )
            {
                int terrainX = store->mCell->getGridX() * 2 + x;
                int terrainY = store->mCell->getGridY() * 2 + y;
                if (mTerrainGroup.getTerrain(terrainX, terrainY) != NULL)
                    mTerrainGroup.unloadTerrain(terrainX, terrainY);
            }
        }
    }

    //----------------------------------------------------------------------------------------------

    void TerrainManager::initTerrainTextures(Terrain::ImportData* terrainData,
                                             int cellX, int cellY,
                                             int fromX, int fromY, int size,
                                             std::map<uint16_t, int>& indexes, size_t plugin)
    {
        // FIXME: In a multiple esm configuration, we have multiple palettes. Since this code
        //  crosses cell boundaries, we no longer have a unique terrain palette. Instead, we need
        //  to adopt the following code for a dynamic palette. And this is evil - the current design
        //  does not work well for this task...

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
        int num = MWBase::Environment::get().getWorld()->getStore().get<ESM::LandTexture>().getSize(plugin);
        std::set<uint16_t> ltexIndexes;
        for ( int y = fromY - 1; y < fromY + size + 1; y++ )
        {
            for ( int x = fromX - 1; x < fromX + size + 1; x++ )
            {
                int idx = getLtexIndexAt(cellX, cellY, x, y);
                // This is a quick hack to prevent the program from trying to fetch textures
                //  from a neighboring cell, which might originate from a different plugin,
                //  and use a separate texture palette. Right now, we simply cast it to the
                //  default texture (i.e. 0).
                if (idx > num)
                  idx = 0;
                ltexIndexes.insert(idx);
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
            uint16_t ltexIndex = *iter;
            //this is the base texture, so we can ignore this at present
            if ( ltexIndex == baseTexture )
            {
                continue;
            }

            const std::map<uint16_t, int>::const_iterator it = indexes.find(ltexIndex);

            if ( it == indexes.end() )
            {
                //NB: All vtex ids are +1 compared to the ltex ids

                const MWWorld::Store<ESM::LandTexture> &ltexStore =
                    MWBase::Environment::get().getWorld()->getStore().get<ESM::LandTexture>();

                // NOTE: using the quick hack above, we should no longer end up with textures indices
                //  that are out of bounds. However, I haven't updated the test to a multi-palette
                //  system yet. We probably need more work here, so we skip it for now.
                //assert( (int)ltexStore.getSize() >= (int)ltexIndex - 1 &&
                       //"LAND.VTEX must be within the bounds of the LTEX array");

                std::string texture;
                if ( ltexIndex == 0 )
                {
                    texture = "_land_default.dds";
                }
                else
                {
                    texture = ltexStore.search(ltexIndex-1, plugin)->mTexture;
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
                                              int cellX, int cellY,
                                              int fromX, int fromY, int size,
                                              const std::map<uint16_t, int>& indexes)
    {
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
                const uint16_t ltexIndex = getLtexIndexAt(cellX, cellY, texX, texY);

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

                for ( int y = -1; y < splatSize + 1; y++ )
                {
                    for ( int x = -1; x < splatSize + 1; x++ )
                    {

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

    int TerrainManager::getLtexIndexAt(int cellX, int cellY,
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


        ESM::Land* land =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Land>().search(cellX, cellY);
        if ( land != NULL )
        {
            if (!land->isDataLoaded(ESM::Land::DATA_VTEX))
            {
                land->loadData(ESM::Land::DATA_VTEX);
            }

            return land->mLandData
                       ->mTextures[y * ESM::Land::LAND_TEXTURE_SIZE + x];
        }
        else
        {
            return 0;
        }
    }

    //----------------------------------------------------------------------------------------------

    TexturePtr TerrainManager::getVertexColours(ESM::Land* land,
                                                int cellX, int cellY,
                                                int fromX, int fromY, int size)
    {
        TextureManager* const texMgr = TextureManager::getSingletonPtr();

        const std::string colourTextureName = "VtexColours_" +
                                              boost::lexical_cast<std::string>(cellX) +
                                              "_" +
                                              boost::lexical_cast<std::string>(cellY) +
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

        if ( land != NULL )
        {
            const char* const colours = land->mLandData->mColours;
            for ( int y = 0; y < size; y++ )
            {
                for ( int x = 0; x < size; x++ )
                {
                    const size_t colourOffset = (y+fromY)*3*65 + (x+fromX)*3;

                    assert( colourOffset < 65*65*3 &&
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
        }
        else
        {
            for ( int y = 0; y < size; y++ )
            {
                for ( int x = 0; x < size; x++ )
                {
                    for ( int k = 0; k < 3; k++ )
                    {
                        *pDest++ = 0;
                    }
                }
            }
        }

        pixelBuffer->unlock();

        return tex;
    }

}
