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
        for (int x = 0; x < ESM::Land::LAND_SIZE; ++x)
        {
            for (int y = 0; y < ESM::Land::LAND_SIZE; ++y)
            {
                mAlteredHeight[y*ESM::Land::LAND_SIZE + x] = 0;
            }
        }
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

    float* TerrainStorage::getAlteredHeights()
    {
        return mAlteredHeight;
    }

    float* TerrainStorage::getAlteredHeight(int inCellX, int inCellY)
    {
        return &mAlteredHeight[inCellY*ESM::Land::LAND_SIZE + inCellX];
    }

    void TerrainStorage::fillVertexBuffers (int lodLevel, float size, const osg::Vec2f& center,
                                            osg::ref_ptr<osg::Vec3Array> positions,
                                            osg::ref_ptr<osg::Vec3Array> normals,
                                            osg::ref_ptr<osg::Vec4ubArray> colours)
    {
        // LOD level n means every 2^n-th vertex is kept
        size_t increment = static_cast<size_t>(1) << lodLevel;

        osg::Vec2f origin = center - osg::Vec2f(size/2.f, size/2.f);

        int startCellX = static_cast<int>(std::floor(origin.x()));
        int startCellY = static_cast<int>(std::floor(origin.y()));

        size_t numVerts = static_cast<size_t>(size*(ESM::Land::LAND_SIZE - 1) / increment + 1);

        positions->resize(numVerts*numVerts);
        normals->resize(numVerts*numVerts);
        colours->resize(numVerts*numVerts);

        osg::Vec3f normal;
        osg::Vec4ub color;

        float vertY = 0;
        float vertX = 0;

        ESMTerrain::LandCache cache;

        float vertY_ = 0; // of current cell corner
        for (int cellY = startCellY; cellY < startCellY + std::ceil(size); ++cellY)
        {
            float vertX_ = 0; // of current cell corner
            for (int cellX = startCellX; cellX < startCellX + std::ceil(size); ++cellX)
            {
                const ESMTerrain::LandObject* land = ESMTerrain::Storage::getLand(cellX, cellY, cache);
                const ESM::Land::LandData *heightData = 0;
                const ESM::Land::LandData *normalData = 0;
                const ESM::Land::LandData *colourData = 0;
                if (land)
                {
                    heightData = land->getData(ESM::Land::DATA_VHGT);
                    normalData = land->getData(ESM::Land::DATA_VNML);
                    colourData = land->getData(ESM::Land::DATA_VCLR);
                }

                int rowStart = 0;
                int colStart = 0;
                // Skip the first row / column unless we're at a chunk edge,
                // since this row / column is already contained in a previous cell
                // This is only relevant if we're creating a chunk spanning multiple cells
                if (vertY_ != 0)
                    colStart += increment;
                if (vertX_ != 0)
                    rowStart += increment;

                // Only relevant for chunks smaller than (contained in) one cell
                rowStart += (origin.x() - startCellX) * ESM::Land::LAND_SIZE;
                colStart += (origin.y() - startCellY) * ESM::Land::LAND_SIZE;
                int rowEnd = std::min(static_cast<int>(rowStart + std::min(1.f, size) * (ESM::Land::LAND_SIZE-1) + 1), static_cast<int>(ESM::Land::LAND_SIZE));
                int colEnd = std::min(static_cast<int>(colStart + std::min(1.f, size) * (ESM::Land::LAND_SIZE-1) + 1), static_cast<int>(ESM::Land::LAND_SIZE));

                vertY = vertY_;
                for (int col=colStart; col<colEnd; col += increment)
                {
                    vertX = vertX_;
                    for (int row=rowStart; row<rowEnd; row += increment)
                    {
                        int srcArrayIndex = col*ESM::Land::LAND_SIZE*3+row*3;

                        assert(row >= 0 && row < ESM::Land::LAND_SIZE);
                        assert(col >= 0 && col < ESM::Land::LAND_SIZE);

                        assert (vertX < numVerts);
                        assert (vertY < numVerts);

                        float height = defaultHeight;
                        if (heightData)
                            height = heightData->mHeights[col*ESM::Land::LAND_SIZE + row];

                        (*positions)[static_cast<unsigned int>(vertX*numVerts + vertY)]
                            = osg::Vec3f((vertX / float(numVerts - 1) - 0.5f) * size * Constants::CellSizeInUnits,
                                         (vertY / float(numVerts - 1) - 0.5f) * size * Constants::CellSizeInUnits,
                                         height + mAlteredHeight[static_cast<unsigned int>(col*ESM::Land::LAND_SIZE + row)]);

                        if (normalData)
                        {
                            for (int i=0; i<3; ++i)
                                normal[i] = normalData->mNormals[srcArrayIndex+i];

                            normal.normalize();
                        }
                        else
                            normal = osg::Vec3f(0,0,1);

                        // Normals apparently don't connect seamlessly between cells
                        if (col == ESM::Land::LAND_SIZE-1 || row == ESM::Land::LAND_SIZE-1)
                            fixNormal(normal, cellX, cellY, col, row, cache);

                        // some corner normals appear to be complete garbage (z < 0)
                        if ((row == 0 || row == ESM::Land::LAND_SIZE-1) && (col == 0 || col == ESM::Land::LAND_SIZE-1))
                            averageNormal(normal, cellX, cellY, col, row, cache);

                        assert(normal.z() > 0);

                        (*normals)[static_cast<unsigned int>(vertX*numVerts + vertY)] = normal;

                        if (colourData)
                        {
                            for (int i=0; i<3; ++i)
                                color[i] = colourData->mColours[srcArrayIndex+i];
                        }
                        else
                        {
                            color.r() = 255;
                            color.g() = 255;
                            color.b() = 255;
                        }

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

                        // Unlike normals, colors mostly connect seamlessly between cells, but not always...
                        if (col == ESM::Land::LAND_SIZE-1 || row == ESM::Land::LAND_SIZE-1)
                            fixColour(color, cellX, cellY, col, row, cache);

                        color.a() = 255;

                        (*colours)[static_cast<unsigned int>(vertX*numVerts + vertY)] = color;

                        ++vertX;
                    }
                    ++vertY;
                }
                vertX_ = vertX;
            }
            vertY_ = vertY;

            assert(vertX_ == numVerts); // Ensure we covered whole area
        }
        assert(vertY_ == numVerts);  // Ensure we covered whole area*/
    }

    void TerrainStorage::getBounds(float &minX, float &maxX, float &minY, float &maxY)
    {
        // not needed at the moment - this returns the bounds of the whole world, but we only edit individual cells
        throw std::runtime_error("getBounds not implemented");
    }

}
