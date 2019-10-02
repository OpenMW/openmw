#ifndef OPENCS_RENDER_TERRAINSTORAGE_H
#define OPENCS_RENDER_TERRAINSTORAGE_H

#include <array>

#include <components/esmterrain/storage.hpp>

#include "../../model/world/data.hpp"

namespace CSVRender
{
    class LandCache;

    /**
     * @brief A bridge between the terrain component and OpenCS's terrain data storage.
     */
    class TerrainStorage : public ESMTerrain::Storage
    {
    public:
        TerrainStorage(const CSMWorld::Data& data);
        std::array<float, ESM::Land::LAND_SIZE * ESM::Land::LAND_SIZE> mAlteredHeight;
        void setAlteredHeight(int inCellX, int inCellY, float heightMap);
        void resetHeights();
        float getSumOfAlteredAndTrueHeight(int cellX, int cellY, int inCellX, int inCellY);
        float* getAlteredHeight(int inCellX, int inCellY);

    private:
        const CSMWorld::Data& mData;

        virtual osg::ref_ptr<const ESMTerrain::LandObject> getLand (int cellX, int cellY) override;
        virtual const ESM::LandTexture* getLandTexture(int index, short plugin) override;

        /// Draws temporarily altered land (transient change support)
        void fillVertexBuffers (int lodLevel, float size, const osg::Vec2f& center,
                        osg::ref_ptr<osg::Vec3Array> positions,
                        osg::ref_ptr<osg::Vec3Array> normals,
                        osg::ref_ptr<osg::Vec4ubArray> colours) override;

        virtual void getBounds(float& minX, float& maxX, float& minY, float& maxY) override;
    };

}

#endif
