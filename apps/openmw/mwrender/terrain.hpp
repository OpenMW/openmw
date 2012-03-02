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
     *
     * This currently has two options as to how the terrain is rendered, one
     * is that one cell is rendered as one Ogre::Terrain and the other that
     * it is rendered as 4 Ogre::Terrain segments
     *
     * Splitting it up into segments has the following advantages
     * * Seems to be faster
     * * Terrain can now be culled more aggressivly using view frustram culling
     * * We don't hit splat limits as much
     */
    class TerrainManager{
    public:
        TerrainManager(Ogre::SceneManager*);
        virtual ~TerrainManager();

        void setDiffuse(const Ogre::ColourValue& diffuse);
        void setAmbient(const Ogre::ColourValue& ambient);

        void cellAdded(MWWorld::Ptr::CellStore* store);
        void cellRemoved(MWWorld::Ptr::CellStore* store);
    private:
        Ogre::TerrainGlobalOptions* mTerrainGlobals;
        Ogre::TerrainGroup* mTerrainGroup;

        /**
         * Should each cell be split into a further four Ogre::Terrain objects
         *
         * This has the advantage that it is possible to cull more terrain and
         * we are more likly to be able to be able to fit all the required splats
         * in (Ogre's default material generator only works with about 6 textures)
         */
        static const bool SPLIT_TERRAIN = true;

        /**
         * The length in verticies of a single terrain block.
         * This takes into account the SPLIT_TERRAIN option
         */
        int mLandSize;

        /**
         * The length in game units of a single terrain block.
         * This takes into account the SPLIT_TERRAIN option
         */
        int mRealSize;

        /**
         * The distance that the current cell should be shaded into the neighbouring
         * texture. The distance is in terms of the splat size of a texture
         */
        static const float TERRAIN_SHADE_DISTANCE = 0.25f;

        /**
         * Setups up the list of textures for part of a cell, using indexes as
         * an output to create a mapping of MW LtexIndex to the relevant terrain
         * layer
         *
         * @param terrainData the terrain data to setup the textures for
         * @param store the cell store for the given terrain cell
         * @param fromX the ltex index in the current cell to start making the texture from
         * @param fromY the ltex index in the current cell to start making the texture from
         * @param size the size (number of splats) to get
         * @param indexes a mapping of ltex index to the terrain texture layer that
         *          can be used by initTerrainBlendMaps
         */
        void initTerrainTextures(Ogre::Terrain::ImportData* terrainData,
                                 MWWorld::Ptr::CellStore* store,
                                 int fromX, int fromY, int size,
                                 std::map<uint16_t, int>& indexes);

        /**
         * Creates the blend (splatting maps) for the given terrain from the ltex data.
         *
         * @param terrain the terrain object for the current cell
         * @param store the cell store for the given terrain cell
         * @param fromX the ltex index in the current cell to start making the texture from
         * @param fromY the ltex index in the current cell to start making the texture from
         * @param size the size (number of splats) to get
         * @param indexes the mapping of ltex to blend map produced by initTerrainTextures
         */
        void initTerrainBlendMaps(Ogre::Terrain* terrain,
                                  MWWorld::Ptr::CellStore* store,
                                  int fromX, int fromY, int size,
                                  const std::map<uint16_t, int>& indexes);

        /**
         * Gets a LTEX index at the given point, assuming the current cell
         * starts at (0,0). This supports getting values from the surrounding
         * cells so negative x, y is acceptable
         *
         * @param store the cell store for the current cell
         * @param x, y the splat position of the ltex index to get relative to the
         *             first splat of the current cell
         */
        int getLtexIndexAt(MWWorld::Ptr::CellStore* store, int x, int y);

        /**
         * Retrives the texture that is the normal and parallax map for the
         * terrain. If it doesn't exist a blank texture is used
         *
         * The file name of the texture should be the same as the file name of
         * the base diffuse texture, but with _n appended before the extension
         *
         * @param fileName the name of the *diffuse* texture
         */
        Ogre::TexturePtr getNormalDisp(const std::string& fileName);

        /**
         * Due to the fact that Ogre terrain doesn't support vertex colours
         * we have to generate them manually
         *
         * @param store the cell store for the given terrain cell
         * @param fromX the *vertex* index in the current cell to start making texture from
         * @param fromY the *vertex* index in the current cell to start making the texture from
         * @param size the size (number of vertexes) to get
         */
        Ogre::TexturePtr getVertexColours(MWWorld::Ptr::CellStore* store,
                                          int fromX, int fromY, int size);
    };

}

#endif // _GAME_RENDER_TERRAIN_H
