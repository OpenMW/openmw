#ifndef OPENCS_RENDER_TERRAINSTORAGE_H
#define OPENCS_RENDER_TERRAINSTORAGE_H

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
    private:
        const CSMWorld::Data& mData;

        virtual osg::ref_ptr<const ESMTerrain::LandObject> getLand (int cellX, int cellY) override;
        virtual const ESM::LandTexture* getLandTexture(int index, short plugin) override;

        virtual void getBounds(float& minX, float& maxX, float& minY, float& maxY) override;
    };

}

#endif
