/*
 * Copyright (c) 2015 scrawl <scrawl@baseoftrash.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef COMPONENTS_TERRAIN_TERRAINGRID_H
#define COMPONENTS_TERRAIN_TERRAINGRID_H

#include "world.hpp"
#include "material.hpp"

namespace Terrain
{
    class Chunk;

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
