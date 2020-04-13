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
        void setAlteredHeight(int inCellX, int inCellY, float heightMap);
        void resetHeights();

        virtual bool useAlteration() const { return true; }
        float getSumOfAlteredAndTrueHeight(int cellX, int cellY, int inCellX, int inCellY);
        float* getAlteredHeight(int inCellX, int inCellY);

    private:
        const CSMWorld::Data& mData;
        std::array<float, ESM::Land::LAND_SIZE * ESM::Land::LAND_SIZE> mAlteredHeight;

        osg::ref_ptr<const ESMTerrain::LandObject> getLand (int cellX, int cellY) final;
        const ESM::LandTexture* getLandTexture(int index, short plugin) final;

        void getBounds(float& minX, float& maxX, float& minY, float& maxY) final;

        int getThisHeight(int col, int row, const ESM::Land::LandData *heightData) const;
        int getLeftHeight(int col, int row, const ESM::Land::LandData *heightData) const;
        int getRightHeight(int col, int row, const ESM::Land::LandData *heightData) const;
        int getUpHeight(int col, int row, const ESM::Land::LandData *heightData) const;
        int getDownHeight(int col, int row, const ESM::Land::LandData *heightData) const;
        int getHeightDifferenceToLeft(int col, int row, const ESM::Land::LandData *heightData) const;
        int getHeightDifferenceToRight(int col, int row, const ESM::Land::LandData *heightData) const;
        int getHeightDifferenceToUp(int col, int row, const ESM::Land::LandData *heightData) const;
        int getHeightDifferenceToDown(int col, int row, const ESM::Land::LandData *heightData) const;
        bool leftOrUpIsOverTheLimit(int col, int row, int heightWarningLimit, const ESM::Land::LandData *heightData) const;
        bool rightOrDownIsOverTheLimit(int col, int row, int heightWarningLimit, const ESM::Land::LandData *heightData) const;

        void adjustColor(int col, int row, const ESM::Land::LandData *heightData, osg::Vec4ub& color) const final;
        float getAlteredHeight(int col, int row) const final;
    };

}

#endif
