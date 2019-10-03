#ifndef OPENCS_RENDER_TERRAINSTORAGE_H
#define OPENCS_RENDER_TERRAINSTORAGE_H

#include <array>

#include <components/esmterrain/storage.hpp>

#include "../../model/world/data.hpp"

namespace CSVRender
{
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

        virtual void getBounds(float& minX, float& maxX, float& minY, float& maxY) override;

        void adjustColor(int col, int row, const ESM::Land::LandData *heightData, osg::Vec4ub& color) const override;
        float getAlteredHeight(int col, int row) const override;
    };

}

#endif
