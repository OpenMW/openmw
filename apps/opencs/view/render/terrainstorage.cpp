#include "terrainstorage.hpp"

namespace CSVRender
{

    TerrainStorage::TerrainStorage(const CSMWorld::Data &data)
        : mData(data)
    {
    }

    ESM::Land* TerrainStorage::getLand(int cellX, int cellY)
    {
        std::ostringstream stream;
        stream << "#" << cellX << " " << cellY;

        // The cell isn't guaranteed to have Land. This is because the terrain implementation
        // has to wrap the vertices of the last row and column to the next cell, which may be a nonexisting cell
        int index = mData.getLand().searchId(stream.str());
        if (index == -1)
            return NULL;

        ESM::Land* land = mData.getLand().getRecord(index).get().mLand.get();
        int mask = ESM::Land::DATA_VHGT | ESM::Land::DATA_VNML | ESM::Land::DATA_VCLR | ESM::Land::DATA_VTEX;
        if (!land->isDataLoaded(mask))
            land->loadData(mask);
        return land;
    }

    const ESM::LandTexture* TerrainStorage::getLandTexture(int index, short plugin)
    {
        std::ostringstream stream;
        stream << index << "_" << plugin;

        return &mData.getLandTextures().getRecord(stream.str()).get();
    }

    void TerrainStorage::getBounds(float &minX, float &maxX, float &minY, float &maxY)
    {
        // not needed at the moment - this returns the bounds of the whole world, but we only edit individual cells
        throw std::runtime_error("getBounds not implemented");
    }

}
