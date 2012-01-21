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

        mTerrainGroup->defineTerrain(x, y, &terrainData);

        mTerrainGroup->loadTerrain(x, y, true);
    }

    void TerrainManager::cellRemoved(MWWorld::Ptr::CellStore *store)
    {
        mTerrainGroup->removeTerrain(store->cell->getGridX(),
                                     store->cell->getGridY());
    }

}
