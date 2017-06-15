#include "terrainstorage.hpp"

namespace CSVRender
{

    TerrainStorage::TerrainStorage(const CSMWorld::Data &data)
        : ESMTerrain::Storage(data.getResourceSystem()->getVFS())
        , mData(data)
    {
    }

    osg::ref_ptr<const ESMTerrain::LandObject> TerrainStorage::getLand(int cellX, int cellY)
    {
        std::ostringstream stream;
        stream << "#" << cellX << " " << cellY;

        // The cell isn't guaranteed to have Land. This is because the terrain implementation
        // has to wrap the vertices of the last row and column to the next cell, which may be a nonexisting cell
        int index = mData.getLand().searchId(stream.str());
        if (index == -1)
            return NULL;

        const ESM::Land& land = mData.getLand().getRecord(index).get();
        return new ESMTerrain::LandObject(&land, ESM::Land::DATA_VHGT | ESM::Land::DATA_VNML | ESM::Land::DATA_VCLR | ESM::Land::DATA_VTEX);
    }

    const ESM::LandTexture* TerrainStorage::getLandTexture(int index, short plugin)
    {
        int numRecords = mData.getLandTextures().getSize();

        for (int i=0; i<numRecords; ++i)
        {
            const CSMWorld::LandTexture* ltex = &mData.getLandTextures().getRecord(i).get();
            if (ltex->mIndex == index && ltex->mPluginIndex == plugin)
                return ltex;
        }

        return NULL;
    }

    void TerrainStorage::getBounds(float &minX, float &maxX, float &minY, float &maxY)
    {
        // not needed at the moment - this returns the bounds of the whole world, but we only edit individual cells
        throw std::runtime_error("getBounds not implemented");
    }

}
