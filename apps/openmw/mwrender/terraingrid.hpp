#ifndef OPENMW_MWRENDER_TERRAINGRID_H
#define OPENMW_MWRENDER_TERRAINGRID_H

#include <components/terrain/world.hpp>
#include <components/terrain/material.hpp>

namespace Terrain
{
    class Chunk;
}

namespace MWRender
{

    struct GridElement
    {
        Ogre::SceneNode* mSceneNode;

        Terrain::MaterialGenerator mMaterialGenerator;

        Terrain::Chunk* mChunk;
    };

    /// @brief Simple terrain implementation that loads cells in a grid, with no LOD
    class TerrainGrid : public Terrain::World
    {
    public:
        /// @note takes ownership of \a storage
        /// @param sceneMgr scene manager to use
        /// @param storage Storage instance to get terrain data from (heights, normals, colors, textures..)
        /// @param visbilityFlags visibility flags for the created meshes
        /// @param shaders Whether to use splatting shader, or multi-pass fixed function splatting. Shader is usually
        ///         faster so this is just here for compatibility.
        /// @param align The align of the terrain, see Alignment enum
        TerrainGrid(Ogre::SceneManager* sceneMgr,
                Terrain::Storage* storage, int visibilityFlags, bool shaders, Terrain::Alignment align);
        ~TerrainGrid();

        /// Update chunk LODs according to this camera position
        virtual void update (const Ogre::Vector3& cameraPos);

        virtual void loadCell(int x, int y);
        virtual void unloadCell(int x, int y);

        /// Get the world bounding box of a chunk of terrain centered at \a center
        virtual Ogre::AxisAlignedBox getWorldBoundingBox (const Ogre::Vector2& center);

        /// Show or hide the whole terrain
        /// @note this setting may be invalidated once you call Terrain::update, so do not call it while the terrain should be hidden
        virtual void setVisible(bool visible);
        virtual bool getVisible();

        /// Recreate materials used by terrain chunks. This should be called whenever settings of
        /// the material factory are changed. (Relying on the factory to update those materials is not
        /// enough, since turning a feature on/off can change the number of texture units available for layer/blend
        /// textures, and to properly respond to this we may need to change the structure of the material, such as
        /// adding or removing passes. This can only be achieved by a full rebuild.)
        virtual void applyMaterials(bool shadows, bool splitShadows);

        /// Wait until all background loading is complete.
        virtual void syncLoad();

    private:
        void updateMaterial (GridElement& element);

        typedef std::map<std::pair<int, int>, GridElement> Grid;
        Grid mGrid;

        Ogre::SceneNode* mRootNode;
        bool mVisible;
    };

}

#endif
