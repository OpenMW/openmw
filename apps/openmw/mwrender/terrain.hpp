#ifndef _GAME_RENDER_TERRAIN_H
#define _GAME_RENDER_TERRAIN_H

#include <OgreTerrain.h>

#include "../mwworld/ptr.hpp"

namespace Ogre{
    class SceneManager;
    class TerrainGroup;
    class TerrainGlobalOptions;
    class Terrain;
}

namespace MWRender{

    /**
     * Implements the Morrowind terrain using the Ogre Terrain Component
     */
    class TerrainManager{
    public:
        TerrainManager(Ogre::SceneManager*);
        virtual ~TerrainManager();

        void cellAdded(MWWorld::Ptr::CellStore* store);
        void cellRemoved(MWWorld::Ptr::CellStore* store);
    private:
        Ogre::TerrainGlobalOptions* mTerrainGlobals;
        Ogre::TerrainGroup* mTerrainGroup;

        /**
         * The distance that the current cell should be shaded into the neighbouring
         * texture. The distance is in terms of the splat size of a texture
         */
        static const float TERRAIN_SHADE_DISTANCE = 0.5;

        /**
         * Setups up the list of textures for the cell
         * @param terrainData the terrain data to setup the textures for
         * @param indexes a mapping of ltex index to the terrain texture layer that
         *          can be used by initTerrainBlendMaps
         */
        void initTerrainTextures(Ogre::Terrain::ImportData* terrainData,
                                 MWWorld::Ptr::CellStore* store,
                                 std::map<uint16_t, int>& indexes);

        /**
         * Creates the blend (splatting maps) for the given terrain from the ltex data.
         * @param terrain the terrain object for the current cell
         * @param indexes the mapping of ltex to blend map produced by initTerrainTextures
         */
        void initTerrainBlendMaps(Ogre::Terrain* terrain,
                                  MWWorld::Ptr::CellStore* store,
                                  const std::map<uint16_t, int>& indexes);
    };

}

#endif // _GAME_RENDER_TERRAIN_H
