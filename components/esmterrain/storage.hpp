#ifndef COMPONENTS_ESM_TERRAIN_STORAGE_H
#define COMPONENTS_ESM_TERRAIN_STORAGE_H

#include <components/terrain/storage.hpp>

#include <components/esm/loadland.hpp>
#include <components/esm/loadltex.hpp>

namespace ESMTerrain
{

    /// @brief Feeds data from ESM terrain records (ESM::Land, ESM::LandTexture)
    ///        into the terrain component, converting it on the fly as needed.
    class Storage : public Terrain::Storage
    {
    private:

        // Not implemented in this class, because we need different Store implementations for game and editor
        virtual ESM::Land* getLand (int cellX, int cellY) = 0;
        virtual const ESM::LandTexture* getLandTexture(int index, short plugin) = 0;

    public:

        // Not implemented in this class, because we need different Store implementations for game and editor
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
        virtual bool getMinMaxHeights (float size, const Ogre::Vector2& center, float& min, float& max);

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
                                std::vector<Ogre::uint8>& colours);

        /// Create textures holding layer blend values for a terrain chunk.
        /// @note The terrain chunk shouldn't be larger than one cell since otherwise we might
        ///       have to do a ridiculous amount of different layers. For larger chunks, composite maps should be used.
        /// @note May be called from background threads.
        /// @param chunkSize size of the terrain chunk in cell units
        /// @param chunkCenter center of the chunk in cell units
        /// @param pack Whether to pack blend values for up to 4 layers into one texture (one in each channel) -
        ///        otherwise, each texture contains blend values for one layer only. Shader-based rendering
        ///        can utilize packing, FFP can't.
        /// @param blendmaps created blendmaps will be written here
        /// @param layerList names of the layer textures used will be written here
        virtual void getBlendmaps (float chunkSize, const Ogre::Vector2& chunkCenter, bool pack,
                           std::vector<Ogre::PixelBox>& blendmaps,
                           std::vector<Terrain::LayerInfo>& layerList);

        /// Retrieve pixel data for textures holding layer blend values for terrain chunks and layer texture information.
        /// This variant is provided to eliminate the overhead of virtual function calls when retrieving a large number of blendmaps at once.
        /// @note The terrain chunks shouldn't be larger than one cell since otherwise we might
        ///       have to do a ridiculous amount of different layers. For larger chunks, composite maps should be used.
        /// @note May be called from background threads.
        /// @param nodes A collection of nodes for which to retrieve the aforementioned data
        /// @param out Output vector
        /// @param pack Whether to pack blend values for up to 4 layers into one texture (one in each channel) -
        ///        otherwise, each texture contains blend values for one layer only. Shader-based rendering
        ///        can utilize packing, FFP can't.
        virtual void getBlendmaps (const std::vector<Terrain::QuadTreeNode*>& nodes, std::vector<Terrain::LayerCollection>& out, bool pack);

        virtual float getHeightAt (const Ogre::Vector3& worldPos);

        virtual Terrain::LayerInfo getDefaultLayer();

        /// Get the transformation factor for mapping cell units to world units.
        virtual float getCellWorldSize();

        /// Get the number of vertices on one side for each cell. Should be (power of two)+1
        virtual int getCellVertices();

    private:
        void fixNormal (Ogre::Vector3& normal, int cellX, int cellY, int col, int row);
        void fixColour (Ogre::ColourValue& colour, int cellX, int cellY, int col, int row);
        void averageNormal (Ogre::Vector3& normal, int cellX, int cellY, int col, int row);

        float getVertexHeight (const ESM::Land* land, int x, int y);

        // Since plugins can define new texture palettes, we need to know the plugin index too
        // in order to retrieve the correct texture name.
        // pair  <texture id, plugin id>
        typedef std::pair<short, short> UniqueTextureId;

        UniqueTextureId getVtexIndexAt(int cellX, int cellY,
                                               int x, int y);
        std::string getTextureName (UniqueTextureId id);

        std::map<std::string, Terrain::LayerInfo> mLayerInfoMap;

        Terrain::LayerInfo getLayerInfo(const std::string& texture);

        // Non-virtual
        void getBlendmapsImpl (float chunkSize, const Ogre::Vector2& chunkCenter, bool pack,
                           std::vector<Ogre::PixelBox>& blendmaps,
                           std::vector<Terrain::LayerInfo>& layerList);
    };

}

#endif
