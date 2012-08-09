#ifndef _GAME_RENDER_TERRAIN_H
#define _GAME_RENDER_TERRAIN_H

#include <OgreTerrain.h>
#include <OgreTerrainGroup.h>

#include <components/esm/loadland.hpp>

#include "terrainmaterial.hpp"

namespace Ogre{
    class SceneManager;
    class TerrainGroup;
    class TerrainGlobalOptions;
    class Terrain;
}

namespace MWWorld
{
    class CellStore;
}

namespace MWRender{

    class RenderingManager;

    /**
     * Implements the Morrowind terrain using the Ogre Terrain Component
     *
     * Each terrain cell is split into four blocks as this leads to an increase
     * in performance and means we don't hit splat limits quite as much
     */
    class TerrainManager{
    public:
        TerrainManager(Ogre::SceneManager* mgr, RenderingManager* rend);
        virtual ~TerrainManager();

        void setDiffuse(const Ogre::ColourValue& diffuse);
        void setAmbient(const Ogre::ColourValue& ambient);

        void cellAdded(MWWorld::CellStore* store);
        void cellRemoved(MWWorld::CellStore* store);
    private:
        Ogre::TerrainGlobalOptions* mTerrainGlobals;
        Ogre::TerrainGroup mTerrainGroup;

        RenderingManager* mRendering;

        TerrainMaterial::Profile* mActiveProfile;

        /**
         * The length in verticies of a single terrain block.
         */
        static const int mLandSize = (ESM::Land::LAND_SIZE - 1)/2 + 1;

        /**
         * The length in game units of a single terrain block.
         */
        static const int mWorldSize = ESM::Land::REAL_SIZE/2;

        /**
         * Setups up the list of textures for part of a cell, using indexes as
         * an output to create a mapping of MW LtexIndex to the relevant terrain
         * layer
         *
         * @param terrainData the terrain data to setup the textures for
         * @param cellX the coord of the cell
         * @param cellY the coord of the cell
         * @param fromX the ltex index in the current cell to start making the texture from
         * @param fromY the ltex index in the current cell to start making the texture from
         * @param size the size (number of splats) to get
         * @param indexes a mapping of ltex index to the terrain texture layer that
         *          can be used by initTerrainBlendMaps
         */
        void initTerrainTextures(Ogre::Terrain::ImportData* terrainData,
                                 int cellX, int cellY,
                                 int fromX, int fromY, int size,
                                 std::map<uint16_t, int>& indexes);

        /**
         * Creates the blend (splatting maps) for the given terrain from the ltex data.
         *
         * @param terrain the terrain object for the current cell
         * @param cellX the coord of the cell
         * @param cellY the coord of the cell
         * @param fromX the ltex index in the current cell to start making the texture from
         * @param fromY the ltex index in the current cell to start making the texture from
         * @param size the size (number of splats) to get
         * @param indexes the mapping of ltex to blend map produced by initTerrainTextures
         */
        void initTerrainBlendMaps(Ogre::Terrain* terrain,
                                  int cellX, int cellY,
                                  int fromX, int fromY, int size,
                                  const std::map<uint16_t, int>& indexes);

        /**
         * Gets a LTEX index at the given point, assuming the current cell
         * starts at (0,0). This supports getting values from the surrounding
         * cells so negative x, y is acceptable
         *
         * @param cellX the coord of the cell
         * @param cellY the coord of the cell
         * @param x, y the splat position of the ltex index to get relative to the
         *             first splat of the current cell
         */
        int getLtexIndexAt(int cellX, int cellY, int x, int y);

        /**
         * Due to the fact that Ogre terrain doesn't support vertex colours
         * we have to generate them manually
         *
         * @param cellX the coord of the cell
         * @param cellY the coord of the cell
         * @param fromX the *vertex* index in the current cell to start making texture from
         * @param fromY the *vertex* index in the current cell to start making the texture from
         * @param size the size (number of vertexes) to get
         */
        Ogre::TexturePtr getVertexColours(ESM::Land* land,
                                          int cellX, int cellY,
                                          int fromX, int fromY, int size);
    };

}

#endif // _GAME_RENDER_TERRAIN_H
