#ifndef OPENCS_RENDER_TERRAINSTORAGE_H
#define OPENCS_RENDER_TERRAINSTORAGE_H

#include <array>

#include <components/esm3/loadland.hpp>
#include <components/esm3/loadltex.hpp>
#include <components/esmterrain/storage.hpp>
#include <osg/ref_ptr>

namespace CSMWorld
{
    class Data;
}
namespace osg
{
    class Vec4ub;
}

namespace CSVRender
{
    /**
     * @brief A bridge between the terrain component and OpenCS's terrain data storage.
     */
    class TerrainStorage : public ESMTerrain::Storage
    {
    public:
        TerrainStorage(const CSMWorld::Data& data);
        void setAlteredHeight(int inCellX, int inCellY, float heightMap);
        void resetHeights();

        bool useAlteration() const override { return true; }
        float getSumOfAlteredAndTrueHeight(int cellX, int cellY, int inCellX, int inCellY);
        float* getAlteredHeight(int inCellX, int inCellY);

    private:
        const CSMWorld::Data& mData;
        std::array<float, ESM::Land::LAND_SIZE * ESM::Land::LAND_SIZE> mAlteredHeight;

        osg::ref_ptr<const ESMTerrain::LandObject> getLand(ESM::ExteriorCellLocation cellLocation) override;
        const ESM::LandTexture* getLandTexture(std::uint16_t index, int plugin) override;

        void getBounds(float& minX, float& maxX, float& minY, float& maxY, ESM::RefId worldspace) override;

        int getThisHeight(int col, int row, std::span<const float> heightData) const;
        int getLeftHeight(int col, int row, std::span<const float> heightData) const;
        int getRightHeight(int col, int row, std::span<const float> heightData) const;
        int getUpHeight(int col, int row, std::span<const float> heightData) const;
        int getDownHeight(int col, int row, std::span<const float> heightData) const;
        int getHeightDifferenceToLeft(int col, int row, std::span<const float> heightData) const;
        int getHeightDifferenceToRight(int col, int row, std::span<const float> heightData) const;
        int getHeightDifferenceToUp(int col, int row, std::span<const float> heightData) const;
        int getHeightDifferenceToDown(int col, int row, std::span<const float> heightData) const;
        bool leftOrUpIsOverTheLimit(int col, int row, int heightWarningLimit, std::span<const float> heightData) const;
        bool rightOrDownIsOverTheLimit(
            int col, int row, int heightWarningLimit, std::span<const float> heightData) const;

        void adjustColor(int col, int row, const ESM::LandData* heightData, osg::Vec4ub& color) const override;
        float getAlteredHeight(int col, int row) const override;
    };

}

#endif
