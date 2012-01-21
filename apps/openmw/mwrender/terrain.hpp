#ifndef _GAME_RENDER_TERRAIN_H
#define _GAME_RENDER_TERRAIN_H

#include "../mwworld/ptr.hpp"

namespace Ogre{
    class SceneManager;
    class TerrainGroup;
    class TerrainGlobalOptions;
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
    };

}

#endif // _GAME_RENDER_TERRAIN_H
