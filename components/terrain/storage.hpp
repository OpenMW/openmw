#ifndef COMPONENTS_TERRAIN_STORAGE_H
#define COMPONENTS_TERRAIN_STORAGE_H

#include <components/esm/loadland.hpp>
#include <components/esm/loadltex.hpp>

#include <OgreAxisAlignedBox.h>

#include <OgreHardwareVertexBuffer.h>

namespace Terrain
{

    struct LayerInfo
    {
        std::string mDiffuseMap;
        std::string mNormalMap;
        bool mParallax; // Height info in normal map alpha channel?
        bool mSpecular; // Specular info in diffuse map alpha channel?
    };

    /// We keep storage of terrain data abstract here since we need different implementations for game and editor
    class Storage
    {
    public:
        virtual ~Storage() {}
    private:
        virtual ESM::Land* getLand (int cellX, int cellY) = 0;
        virtual const ESM::LandTexture* getLandTexture(int index, short plugin) = 0;

    public:
        /// Get bounds of the whole terrain in cell units
        virtual Ogre::AxisAlignedBox getBounds() = 0;

        /// Get the minimum and maximum heights of a terrain chunk.
        /// @note Should only be called for chunks <= 1 cell, i.e. leafs of the quad tree.
        ///        Larger chunks can simply merge AABB of children.
        /// @param size size of the chunk in cell units
        /// @param center center of the chunk in cell units
        /// @param min min height will be stored here
        /// @param max max height will be stored here
        /// @return true if there was data available for this terrain chunk
        bool getMinMaxHeights (float size, const Ogre::Vector2& center, float& min, float& max);

        /// Fill vertex buffers for a terrain chunk.
        /// @param lodLevel LOD level, 0 = most detailed
        /// @param size size of the terrain chunk in cell units
        /// @param center center of the chunk in cell units
        /// @param vertexBuffer buffer to write vertices
        /// @param normalBuffer buffer to write vertex normals
        /// @param colourBuffer buffer to write vertex colours
        void fillVertexBuffers (int lodLevel, float size, const Ogre::Vector2& center,
                                Ogre::HardwareVertexBufferSharedPtr vertexBuffer,
                                Ogre::HardwareVertexBufferSharedPtr normalBuffer,
                                Ogre::HardwareVertexBufferSharedPtr colourBuffer);

        /// Create textures holding layer blend values for a terrain chunk.
        /// @note The terrain chunk shouldn't be larger than one cell since otherwise we might
        ///       have to do a ridiculous amount of different layers. For larger chunks, composite maps should be used.
        /// @param chunkSize size of the terrain chunk in cell units
        /// @param chunkCenter center of the chunk in cell units
        /// @param pack Whether to pack blend values for up to 4 layers into one texture (one in each channel) -
        ///        otherwise, each texture contains blend values for one layer only. Shader-based rendering
        ///        can utilize packing, FFP can't.
        /// @param blendmaps created blendmaps will be written here
        /// @param layerList names of the layer textures used will be written here
        void getBlendmaps (float chunkSize, const Ogre::Vector2& chunkCenter, bool pack,
                           std::vector<Ogre::TexturePtr>& blendmaps,
                           std::vector<LayerInfo>& layerList);

        float getHeightAt (const Ogre::Vector3& worldPos);

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

        std::map<std::string, LayerInfo> mLayerInfoMap;

        LayerInfo getLayerInfo(const std::string& texture);
    };

}

#endif
