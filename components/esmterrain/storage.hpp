#ifndef OPENMW_COMPONENTS_ESMTERRAIN_STORAGE_H
#define OPENMW_COMPONENTS_ESMTERRAIN_STORAGE_H

#include <cassert>
#include <mutex>

#include <components/terrain/defs.hpp>
#include <components/terrain/storage.hpp>

#include <components/esm/esmterrain.hpp>
#include <components/esm/exteriorcelllocation.hpp>
#include <components/esm3/loadltex.hpp>

namespace ESM4
{
    struct Land;
    struct LandTexture;
    struct TextureSet;
}

namespace ESM
{
    class LandData;
}

namespace VFS
{
    class Manager;
}

namespace ESMTerrain
{

    class LandCache;

    /// @brief Wrapper around Land Data with reference counting. The wrapper needs to be held as long as the data is
    /// still in use
    class LandObject : public osg::Object
    {
    public:
        LandObject() = default;
        LandObject(const ESM::Land& land, int loadFlags);
        LandObject(const ESM4::Land& land, int loadFlags);

        META_Object(ESMTerrain, LandObject)

        const ESM::LandData* getData(int flags) const
        {
            if ((mData.getLoadFlags() & flags) != flags)
                return nullptr;

            return &mData;
        }

        int getPlugin() const { return mData.getPlugin(); }

        const Terrain::LayerInfo& getEsm4DefaultLayerInfo() const { return mEsm4DefaultLayerInfo; }

    private:
        ESM::LandData mData;

        Terrain::LayerInfo mEsm4DefaultLayerInfo;

        LandObject(const LandObject& copy, const osg::CopyOp& copyOp);
    };

    // Since plugins can define new texture palettes, we need to know the plugin index too
    // in order to retrieve the correct texture name.
    // pair  <texture id, plugin id>
    using UniqueTextureId = std::pair<std::uint16_t, int>;

    /// @brief Feeds data from ESM terrain records (ESM::Land, ESM::LandTexture)
    ///        into the terrain component, converting it on the fly as needed.
    class Storage : public Terrain::Storage
    {
    public:
        Storage(const VFS::Manager* vfs, std::string_view normalMapPattern = {},
            std::string_view normalHeightMapPattern = {}, bool autoUseNormalMaps = false,
            std::string_view specularMapPattern = {}, bool autoUseSpecularMaps = false);

        // Not implemented in this class, because we need different Store implementations for game and editor
        virtual osg::ref_ptr<const LandObject> getLand(ESM::ExteriorCellLocation cellLocation) = 0;
        virtual const std::string* getLandTexture(std::uint16_t index, int plugin) = 0;

        // Not implemented in this class because requires ESMStore
        virtual const ESM4::LandTexture* getEsm4LandTexture(ESM::RefId ltexId) const { return nullptr; }
        virtual const ESM4::TextureSet* getEsm4TextureSet(ESM::RefId txstId) const { return nullptr; }

        /// Get bounds of the whole terrain in cell units
        void getBounds(float& minX, float& maxX, float& minY, float& maxY, ESM::RefId worldspace) override = 0;

        /// Get the minimum and maximum heights of a terrain region.
        /// @note Will only be called for chunks with size = minBatchSize, i.e. leafs of the quad tree.
        ///        Larger chunks can simply merge AABB of children.
        /// @param size size of the chunk in cell units
        /// @param center center of the chunk in cell units
        /// @param min min height will be stored here
        /// @param max max height will be stored here
        /// @return true if there was data available for this terrain chunk
        bool getMinMaxHeights(
            float size, const osg::Vec2f& center, ESM::RefId worldspace, float& min, float& max) override;

        /// Fill vertex buffers for a terrain chunk.
        /// @note May be called from background threads. Make sure to only call thread-safe functions from here!
        /// @note Vertices should be written in row-major order (a row is defined as parallel to the x-axis).
        ///       The specified positions should be in local space, i.e. relative to the center of the terrain chunk.
        /// @param lodLevel LOD level, 0 = most detailed
        /// @param size size of the terrain chunk in cell units
        /// @param center center of the chunk in cell units
        /// @param positions buffer to write vertices
        /// @param normals buffer to write vertex normals
        /// @param colours buffer to write vertex colours
        void fillVertexBuffers(int lodLevel, float size, const osg::Vec2f& center, ESM::RefId worldspace,
            osg::Vec3Array& positions, osg::Vec3Array& normals, osg::Vec4ubArray& colours) override;

        /// Create textures holding layer blend values for a terrain chunk.
        /// @note The terrain chunk shouldn't be larger than one cell since otherwise we might
        ///       have to do a ridiculous amount of different layers. For larger chunks, composite maps should be used.
        /// @note May be called from background threads.
        /// @param chunkSize size of the terrain chunk in cell units
        /// @param chunkCenter center of the chunk in cell units
        /// @param blendmaps created blendmaps will be written here
        /// @param layerList names of the layer textures used will be written here
        void getBlendmaps(float chunkSize, const osg::Vec2f& chunkCenter, ImageVector& blendmaps,
            std::vector<Terrain::LayerInfo>& layerList, ESM::RefId worldspace) override;

        float getHeightAt(const osg::Vec3f& worldPos, ESM::RefId worldspace) override;

        /// Get the transformation factor for mapping cell units to world units.
        float getCellWorldSize(ESM::RefId worldspace) override;

        /// Get the number of vertices on one side for each cell. Should be (power of two)+1
        int getCellVertices(ESM::RefId worldspace) override;

        int getTextureTileCount(float chunkSize, ESM::RefId worldspace) override;

        float getVertexHeight(const ESM::LandData* data, int x, int y)
        {
            const int landSize = data->getLandSize();
            assert(x < landSize);
            assert(y < landSize);
            return data->getHeights()[y * landSize + x];
        }

    private:
        const VFS::Manager* mVFS;

        inline void fixNormal(
            osg::Vec3f& normal, ESM::ExteriorCellLocation cellLocation, int col, int row, LandCache& cache);
        inline void fixColour(
            osg::Vec4ub& colour, ESM::ExteriorCellLocation cellLocation, int col, int row, LandCache& cache);
        inline void averageNormal(
            osg::Vec3f& normal, ESM::ExteriorCellLocation cellLocation, int col, int row, LandCache& cache);

        inline const LandObject* getLand(ESM::ExteriorCellLocation cellLocation, LandCache& cache);

        virtual bool useAlteration() const { return false; }
        virtual void adjustColor(int col, int row, const ESM::LandData* heightData, osg::Vec4ub& color) const;
        virtual float getAlteredHeight(int col, int row) const;

        VFS::Path::Normalized getTextureName(UniqueTextureId id);

        std::map<VFS::Path::Normalized, Terrain::LayerInfo, std::less<>> mLayerInfoMap;
        std::mutex mLayerInfoMutex;

        std::string mNormalMapPattern;
        std::string mNormalHeightMapPattern;
        bool mAutoUseNormalMaps;

        std::string mSpecularMapPattern;
        bool mAutoUseSpecularMaps;

        Terrain::LayerInfo getLayerInfo(VFS::Path::NormalizedView texture);
        Terrain::LayerInfo getTextureSetLayerInfo(const ESM4::TextureSet& txst);
        Terrain::LayerInfo getLandTextureLayerInfo(ESM::FormId id);

        void getEsm4Blendmaps(float chunkSize, const osg::Vec2f& chunkCenter, ImageVector& blendmaps,
            std::vector<Terrain::LayerInfo>& layerList, ESM::RefId worldspace);
    };

}

#endif
