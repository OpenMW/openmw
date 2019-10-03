#include "terrainstorage.hpp"

#include <set>
#include <memory>

#include "../../model/world/land.hpp"
#include "../../model/world/landtexture.hpp"

#include <components/esmterrain/storage.hpp>
#include <components/debug/debuglog.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/vfs/manager.hpp>

namespace CSVRender
{
    const float defaultHeight = ESM::Land::DEFAULT_HEIGHT;

    TerrainStorage::TerrainStorage(const CSMWorld::Data &data)
        : ESMTerrain::Storage(data.getResourceSystem()->getVFS())
        , mData(data)
    {
        resetHeights();
    }

    osg::ref_ptr<const ESMTerrain::LandObject> TerrainStorage::getLand(int cellX, int cellY)
    {
        // The cell isn't guaranteed to have Land. This is because the terrain implementation
        // has to wrap the vertices of the last row and column to the next cell, which may be a nonexisting cell
        int index = mData.getLand().searchId(CSMWorld::Land::createUniqueRecordId(cellX, cellY));
        if (index == -1)
            return nullptr;

        const ESM::Land& land = mData.getLand().getRecord(index).get();
        return new ESMTerrain::LandObject(&land, ESM::Land::DATA_VHGT | ESM::Land::DATA_VNML | ESM::Land::DATA_VCLR | ESM::Land::DATA_VTEX);
    }

    const ESM::LandTexture* TerrainStorage::getLandTexture(int index, short plugin)
    {
        int row = mData.getLandTextures().searchId(CSMWorld::LandTexture::createUniqueRecordId(plugin, index));
        if (row == -1)
            return nullptr;

        return &mData.getLandTextures().getRecord(row).get();
    }

    void TerrainStorage::setAlteredHeight(int inCellX, int inCellY, float height)
    {
        mAlteredHeight[inCellY*ESM::Land::LAND_SIZE + inCellX] = height - fmod(height, 8); //Limit to divisible by 8 to avoid cell seam breakage
    }

    void TerrainStorage::resetHeights()
    {
        std::fill(std::begin(mAlteredHeight), std::end(mAlteredHeight), 0);
    }

    float TerrainStorage::getSumOfAlteredAndTrueHeight(int cellX, int cellY, int inCellX, int inCellY)
    {
        float height = 0.f;
        osg::ref_ptr<const ESMTerrain::LandObject> land = getLand (cellX, cellY);
        if (land)
        {
            const ESM::Land::LandData* data = land ? land->getData(ESM::Land::DATA_VHGT) : 0;
            if (data) height = getVertexHeight(data, inCellX, inCellY);
        }
        else return height;
        return mAlteredHeight[inCellY*ESM::Land::LAND_SIZE + inCellX] + height;

    }

    float* TerrainStorage::getAlteredHeight(int inCellX, int inCellY)
    {
        return &mAlteredHeight[inCellY*ESM::Land::LAND_SIZE + inCellX];
    }

    void TerrainStorage::getBounds(float &minX, float &maxX, float &minY, float &maxY)
    {
        // not needed at the moment - this returns the bounds of the whole world, but we only edit individual cells
        throw std::runtime_error("getBounds not implemented");
    }

    void TerrainStorage::adjustColor(int col, int row, const ESM::Land::LandData *heightData, osg::Vec4ub& color) const
    {
        // Highlight broken height changes
        if ( ((col > 0 && row > 0) &&
            ((abs(heightData->mHeights[col*ESM::Land::LAND_SIZE + row] +
            mAlteredHeight[static_cast<unsigned int>(col*ESM::Land::LAND_SIZE + row)] -
            (heightData->mHeights[(col)*ESM::Land::LAND_SIZE + row - 1] +
            mAlteredHeight[static_cast<unsigned int>((col)*ESM::Land::LAND_SIZE + row - 1)])) >= 1024 ) ||
            abs(heightData->mHeights[col*ESM::Land::LAND_SIZE + row] +
            mAlteredHeight[static_cast<unsigned int>(col*ESM::Land::LAND_SIZE + row)] -
            (heightData->mHeights[(col - 1)*ESM::Land::LAND_SIZE + row] +
            mAlteredHeight[static_cast<unsigned int>((col - 1)*ESM::Land::LAND_SIZE + row)]))  >= 1024 )) ||
            ((col < ESM::Land::LAND_SIZE - 1 && row < ESM::Land::LAND_SIZE - 1) &&
            ((abs(heightData->mHeights[col*ESM::Land::LAND_SIZE + row] +
            mAlteredHeight[static_cast<unsigned int>(col*ESM::Land::LAND_SIZE + row)] -
            (heightData->mHeights[(col)*ESM::Land::LAND_SIZE + row + 1] +
            mAlteredHeight[static_cast<unsigned int>((col)*ESM::Land::LAND_SIZE + row + 1)])) >= 1024 ) ||
            abs(heightData->mHeights[col*ESM::Land::LAND_SIZE + row] +
            mAlteredHeight[static_cast<unsigned int>(col*ESM::Land::LAND_SIZE + row)] -
            (heightData->mHeights[(col + 1)*ESM::Land::LAND_SIZE + row] +
            mAlteredHeight[static_cast<unsigned int>((col + 1)*ESM::Land::LAND_SIZE + row)]))  >= 1024 )))
        {
            color.r() = 255;
            color.g() = 0;
            color.b() = 0;
        }
    }

    float TerrainStorage::getAlteredHeight(int col, int row) const
    {
        return mAlteredHeight[static_cast<unsigned int>(col*ESM::Land::LAND_SIZE + row)];
    }
}
