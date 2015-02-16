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
#ifndef COMPONENTS_TERRAIN_STORAGE_H
#define COMPONENTS_TERRAIN_STORAGE_H

#include <OgreHardwareVertexBuffer.h>

#include "defs.hpp"

namespace Terrain
{
    /// We keep storage of terrain data abstract here since we need different implementations for game and editor
    class Storage
    {
    public:
        virtual ~Storage() {}

    public:
        /// Get bounds of the whole terrain in cell units
        virtual void getBounds(float& minX, float& maxX, float& minY, float& maxY) = 0;

        /// Get the minimum and maximum heights of a terrain region.
        /// @note Will only be called for chunks with size = minBatchSize, i.e. leafs of the quad tree.
        ///        Larger chunks can simply merge AABB of children.
        /// @param size size of the chunk in cell units
        /// @param center center of the chunk in cell units
        /// @param min min height will be stored here
        /// @param max max height will be stored here
        /// @return true if there was data available for this terrain chunk
        virtual bool getMinMaxHeights (float size, const Ogre::Vector2& center, float& min, float& max) = 0;

        /// Fill vertex buffers for a terrain chunk.
        /// @note May be called from background threads. Make sure to only call thread-safe functions from here!
        /// @note returned colors need to be in render-system specific format! Use RenderSystem::convertColourValue.
        /// @note Vertices should be written in row-major order (a row is defined as parallel to the x-axis).
        ///       The specified positions should be in local space, i.e. relative to the center of the terrain chunk.
        /// @param lodLevel LOD level, 0 = most detailed
        /// @param size size of the terrain chunk in cell units
        /// @param center center of the chunk in cell units
        /// @param positions buffer to write vertices
        /// @param normals buffer to write vertex normals
        /// @param colours buffer to write vertex colours
        virtual void fillVertexBuffers (int lodLevel, float size, const Ogre::Vector2& center, Terrain::Alignment align,
                                std::vector<float>& positions,
                                std::vector<float>& normals,
                                std::vector<Ogre::uint8>& colours) = 0;

        /// Create textures holding layer blend values for a terrain chunk.
        /// @note The terrain chunk shouldn't be larger than one cell since otherwise we might
        ///       have to do a ridiculous amount of different layers. For larger chunks, composite maps should be used.
        /// @note May be called from background threads. Make sure to only call thread-safe functions from here!
        /// @param chunkSize size of the terrain chunk in cell units
        /// @param chunkCenter center of the chunk in cell units
        /// @param pack Whether to pack blend values for up to 4 layers into one texture (one in each channel) -
        ///        otherwise, each texture contains blend values for one layer only. Shader-based rendering
        ///        can utilize packing, FFP can't.
        /// @param blendmaps created blendmaps will be written here
        /// @param layerList names of the layer textures used will be written here
        virtual void getBlendmaps (float chunkSize, const Ogre::Vector2& chunkCenter, bool pack,
                           std::vector<Ogre::PixelBox>& blendmaps,
                           std::vector<LayerInfo>& layerList) = 0;

        /// Retrieve pixel data for textures holding layer blend values for terrain chunks and layer texture information.
        /// This variant is provided to eliminate the overhead of virtual function calls when retrieving a large number of blendmaps at once.
        /// @note The terrain chunks shouldn't be larger than one cell since otherwise we might
        ///       have to do a ridiculous amount of different layers. For larger chunks, composite maps should be used.
        /// @note May be called from background threads. Make sure to only call thread-safe functions from here!
        /// @param nodes A collection of nodes for which to retrieve the aforementioned data
        /// @param out Output vector
        /// @param pack Whether to pack blend values for up to 4 layers into one texture (one in each channel) -
        ///        otherwise, each texture contains blend values for one layer only. Shader-based rendering
        ///        can utilize packing, FFP can't.
        virtual void getBlendmaps (const std::vector<QuadTreeNode*>& nodes, std::vector<LayerCollection>& out, bool pack) = 0;

        virtual float getHeightAt (const Ogre::Vector3& worldPos) = 0;

        virtual LayerInfo getDefaultLayer() = 0;

        /// Get the transformation factor for mapping cell units to world units.
        virtual float getCellWorldSize() = 0;

        /// Get the number of vertices on one side for each cell. Should be (power of two)+1
        virtual int getCellVertices() = 0;
    };

}

#endif
