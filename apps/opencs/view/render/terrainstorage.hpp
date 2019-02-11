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
        float mAlteredHeight[ESM::Land::LAND_SIZE * ESM::Land::LAND_SIZE + ESM::Land::LAND_SIZE];
        void alterHeights(float heightmap[ESM::Land::LAND_SIZE * ESM::Land::LAND_SIZE + ESM::Land::LAND_SIZE]);
        void resetHeights();

    private:
        const CSMWorld::Data& mData;

        virtual osg::ref_ptr<const ESMTerrain::LandObject> getLand (int cellX, int cellY);
        virtual const ESM::LandTexture* getLandTexture(int index, short plugin);

        void fillVertexBuffers (int lodLevel, float size, const osg::Vec2f& center,
                                osg::ref_ptr<osg::Vec3Array> positions,
                                osg::ref_ptr<osg::Vec3Array> normals,
                                osg::ref_ptr<osg::Vec4Array> colours) override;

        virtual void getBounds(float& minX, float& maxX, float& minY, float& maxY);
    };

}

#endif
