#include "storage.hpp"

#include <set>

#include <osg/Image>
#include <osg/Plane>

#include <boost/algorithm/string.hpp>

#include <components/misc/resourcehelpers.hpp>
#include <components/vfs/manager.hpp>

namespace ESMTerrain
{

    Storage::Storage(const VFS::Manager *vfs)
        : mVFS(vfs)
    {
    }

    const ESM::Land::LandData *Storage::getLandData (int cellX, int cellY, int flags)
    {
        if (const ESM::Land *land = getLand (cellX, cellY))
            return land->getLandData (flags);

        return 0;
    }

    bool Storage::getMinMaxHeights(float size, const osg::Vec2f &center, float &min, float &max)
    {
        assert (size <= 1 && "Storage::getMinMaxHeights, chunk size should be <= 1 cell");

        /// \todo investigate if min/max heights should be stored at load time in ESM::Land instead

        osg::Vec2f origin = center - osg::Vec2f(size/2.f, size/2.f);

        int cellX = static_cast<int>(std::floor(origin.x()));
        int cellY = static_cast<int>(std::floor(origin.y()));

        int startRow = (origin.x() - cellX) * ESM::Land::LAND_SIZE;
        int startColumn = (origin.y() - cellY) * ESM::Land::LAND_SIZE;

        int endRow = startRow + size * (ESM::Land::LAND_SIZE-1) + 1;
        int endColumn = startColumn + size * (ESM::Land::LAND_SIZE-1) + 1;

        if (const ESM::Land::LandData *data = getLandData (cellX, cellY, ESM::Land::DATA_VHGT))
        {
            min = std::numeric_limits<float>::max();
            max = -std::numeric_limits<float>::max();
            for (int row=startRow; row<endRow; ++row)
            {
                for (int col=startColumn; col<endColumn; ++col)
                {
                    float h = data->mHeights[col*ESM::Land::LAND_SIZE+row];
                    if (h > max)
                        max = h;
                    if (h < min)
                        min = h;
                }
            }
            return true;
        }

        return false;
    }

    void Storage::fixNormal (osg::Vec3f& normal, int cellX, int cellY, int col, int row)
    {
        while (col >= ESM::Land::LAND_SIZE-1)
        {
            ++cellY;
            col -= ESM::Land::LAND_SIZE-1;
        }
        while (row >= ESM::Land::LAND_SIZE-1)
        {
            ++cellX;
            row -= ESM::Land::LAND_SIZE-1;
        }
        while (col < 0)
        {
            --cellY;
            col += ESM::Land::LAND_SIZE-1;
        }
        while (row < 0)
        {
            --cellX;
            row += ESM::Land::LAND_SIZE-1;
        }

        if (const ESM::Land::LandData *data = getLandData (cellX, cellY, ESM::Land::DATA_VNML))
        {
            normal.x() = data->mNormals[col*ESM::Land::LAND_SIZE*3+row*3];
            normal.y() = data->mNormals[col*ESM::Land::LAND_SIZE*3+row*3+1];
            normal.z() = data->mNormals[col*ESM::Land::LAND_SIZE*3+row*3+2];
            normal.normalize();
        }
        else
            normal = osg::Vec3f(0,0,1);
    }

    void Storage::averageNormal(osg::Vec3f &normal, int cellX, int cellY, int col, int row)
    {
        osg::Vec3f n1,n2,n3,n4;
        fixNormal(n1, cellX, cellY, col+1, row);
        fixNormal(n2, cellX, cellY, col-1, row);
        fixNormal(n3, cellX, cellY, col, row+1);
        fixNormal(n4, cellX, cellY, col, row-1);
        normal = (n1+n2+n3+n4);
        normal.normalize();
    }

    void Storage::fixColour (osg::Vec4f& color, int cellX, int cellY, int col, int row)
    {
        if (col == ESM::Land::LAND_SIZE-1)
        {
            ++cellY;
            col = 0;
        }
        if (row == ESM::Land::LAND_SIZE-1)
        {
            ++cellX;
            row = 0;
        }

        if (const ESM::Land::LandData *data = getLandData (cellX, cellY, ESM::Land::DATA_VCLR))
        {
            color.r() = data->mColours[col*ESM::Land::LAND_SIZE*3+row*3] / 255.f;
            color.g() = data->mColours[col*ESM::Land::LAND_SIZE*3+row*3+1] / 255.f;
            color.b() = data->mColours[col*ESM::Land::LAND_SIZE*3+row*3+2] / 255.f;
        }
        else
        {
            color.r() = 1;
            color.g() = 1;
            color.b() = 1;
        }

    }

    void Storage::fillVertexBuffers (int lodLevel, float size, const osg::Vec2f& center,
                                            osg::ref_ptr<osg::Vec3Array> positions,
                                            osg::ref_ptr<osg::Vec3Array> normals,
                                            osg::ref_ptr<osg::Vec4Array> colours)
    {
        // LOD level n means every 2^n-th vertex is kept
        size_t increment = 1 << lodLevel;

        osg::Vec2f origin = center - osg::Vec2f(size/2.f, size/2.f);

        int startCellX = static_cast<int>(std::floor(origin.x()));
        int startCellY = static_cast<int>(std::floor(origin.y()));

        size_t numVerts = static_cast<size_t>(size*(ESM::Land::LAND_SIZE - 1) / increment + 1);

        positions->resize(numVerts*numVerts);
        normals->resize(numVerts*numVerts);
        colours->resize(numVerts*numVerts);

        osg::Vec3f normal;
        osg::Vec4f color;

        float vertY = 0;
        float vertX = 0;

        float vertY_ = 0; // of current cell corner
        for (int cellY = startCellY; cellY < startCellY + std::ceil(size); ++cellY)
        {
            float vertX_ = 0; // of current cell corner
            for (int cellX = startCellX; cellX < startCellX + std::ceil(size); ++cellX)
            {
                const ESM::Land::LandData *heightData = getLandData (cellX, cellY, ESM::Land::DATA_VHGT);
                const ESM::Land::LandData *normalData = getLandData (cellX, cellY, ESM::Land::DATA_VNML);
                const ESM::Land::LandData *colourData = getLandData (cellX, cellY, ESM::Land::DATA_VCLR);

                int rowStart = 0;
                int colStart = 0;
                // Skip the first row / column unless we're at a chunk edge,
                // since this row / column is already contained in a previous cell
                // This is only relevant if we're creating a chunk spanning multiple cells
                if (colStart == 0 && vertY_ != 0)
                    colStart += increment;
                if (rowStart == 0 && vertX_ != 0)
                    rowStart += increment;

                // Only relevant for chunks smaller than (contained in) one cell
                rowStart += (origin.x() - startCellX) * ESM::Land::LAND_SIZE;
                colStart += (origin.y() - startCellY) * ESM::Land::LAND_SIZE;
                int rowEnd = rowStart + std::min(1.f, size) * (ESM::Land::LAND_SIZE-1) + 1;
                int colEnd = colStart + std::min(1.f, size) * (ESM::Land::LAND_SIZE-1) + 1;

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

                        float height = -2048;
                        if (heightData)
                            height = heightData->mHeights[col*ESM::Land::LAND_SIZE + row];

                        (*positions)[static_cast<unsigned int>(vertX*numVerts + vertY)]
                            = osg::Vec3f((vertX / float(numVerts - 1) - 0.5f) * size * 8192,
                                         (vertY / float(numVerts - 1) - 0.5f) * size * 8192,
                                         height);

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
                            fixNormal(normal, cellX, cellY, col, row);

                        // some corner normals appear to be complete garbage (z < 0)
                        if ((row == 0 || row == ESM::Land::LAND_SIZE-1) && (col == 0 || col == ESM::Land::LAND_SIZE-1))
                            averageNormal(normal, cellX, cellY, col, row);

                        assert(normal.z() > 0);

                        (*normals)[static_cast<unsigned int>(vertX*numVerts + vertY)] = normal;

                        if (colourData)
                        {
                            for (int i=0; i<3; ++i)
                                color[i] = colourData->mColours[srcArrayIndex+i] / 255.f;
                        }
                        else
                        {
                            color.r() = 1;
                            color.g() = 1;
                            color.b() = 1;
                        }

                        // Unlike normals, colors mostly connect seamlessly between cells, but not always...
                        if (col == ESM::Land::LAND_SIZE-1 || row == ESM::Land::LAND_SIZE-1)
                            fixColour(color, cellX, cellY, col, row);

                        color.a() = 1;

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
        assert(vertY_ == numVerts);  // Ensure we covered whole area
    }

    Storage::UniqueTextureId Storage::getVtexIndexAt(int cellX, int cellY,
                                           int x, int y)
    {
        // For the first/last row/column, we need to get the texture from the neighbour cell
        // to get consistent blending at the borders
        --x;
        if (x < 0)
        {
            --cellX;
            x += ESM::Land::LAND_TEXTURE_SIZE;
        }
        if (y >= ESM::Land::LAND_TEXTURE_SIZE) // Y appears to be wrapped from the other side because why the hell not?
        {
            ++cellY;
            y -= ESM::Land::LAND_TEXTURE_SIZE;
        }

        assert(x<ESM::Land::LAND_TEXTURE_SIZE);
        assert(y<ESM::Land::LAND_TEXTURE_SIZE);

        if (const ESM::Land::LandData *data = getLandData (cellX, cellY, ESM::Land::DATA_VTEX))
        {
            int tex = data->mTextures[y * ESM::Land::LAND_TEXTURE_SIZE + x];
            if (tex == 0)
                return std::make_pair(0,0); // vtex 0 is always the base texture, regardless of plugin
            return std::make_pair(tex, getLand (cellX, cellY)->mPlugin);
        }
        else
            return std::make_pair(0,0);
    }

    std::string Storage::getTextureName(UniqueTextureId id)
    {
        if (id.first == 0)
            return "textures\\_land_default.dds"; // Not sure if the default texture really is hardcoded?

        // NB: All vtex ids are +1 compared to the ltex ids
        const ESM::LandTexture* ltex = getLandTexture(id.first-1, id.second);

        // this is needed due to MWs messed up texture handling
        std::string texture = Misc::ResourceHelpers::correctTexturePath(ltex->mTexture, mVFS);

        return texture;
    }

    void Storage::getBlendmaps(float chunkSize, const osg::Vec2f &chunkCenter,
        bool pack, ImageVector &blendmaps, std::vector<Terrain::LayerInfo> &layerList)
    {
        // TODO - blending isn't completely right yet; the blending radius appears to be
        // different at a cell transition (2 vertices, not 4), so we may need to create a larger blendmap
        // and interpolate the rest of the cell by hand? :/

        osg::Vec2f origin = chunkCenter - osg::Vec2f(chunkSize/2.f, chunkSize/2.f);
        int cellX = static_cast<int>(std::floor(origin.x()));
        int cellY = static_cast<int>(std::floor(origin.y()));

        int realTextureSize = ESM::Land::LAND_TEXTURE_SIZE+1; // add 1 to wrap around next cell

        int rowStart = (origin.x() - cellX) * realTextureSize;
        int colStart = (origin.y() - cellY) * realTextureSize;
        int rowEnd = rowStart + chunkSize * (realTextureSize-1) + 1;
        int colEnd = colStart + chunkSize * (realTextureSize-1) + 1;

        assert (rowStart >= 0 && colStart >= 0);
        assert (rowEnd <= realTextureSize);
        assert (colEnd <= realTextureSize);

        // Save the used texture indices so we know the total number of textures
        // and number of required blend maps
        std::set<UniqueTextureId> textureIndices;
        // Due to the way the blending works, the base layer will always shine through in between
        // blend transitions (eg halfway between two texels, both blend values will be 0.5, so 25% of base layer visible).
        // To get a consistent look, we need to make sure to use the same base layer in all cells.
        // So we're always adding _land_default.dds as the base layer here, even if it's not referenced in this cell.
        textureIndices.insert(std::make_pair(0,0));

        for (int y=colStart; y<colEnd; ++y)
            for (int x=rowStart; x<rowEnd; ++x)
            {
                UniqueTextureId id = getVtexIndexAt(cellX, cellY, x, y);
                textureIndices.insert(id);
            }

        // Makes sure the indices are sorted, or rather,
        // retrieved as sorted. This is important to keep the splatting order
        // consistent across cells.
        std::map<UniqueTextureId, int> textureIndicesMap;
        for (std::set<UniqueTextureId>::iterator it = textureIndices.begin(); it != textureIndices.end(); ++it)
        {
            int size = textureIndicesMap.size();
            textureIndicesMap[*it] = size;
            layerList.push_back(getLayerInfo(getTextureName(*it)));
        }

        int numTextures = textureIndices.size();
        // numTextures-1 since the base layer doesn't need blending
        int numBlendmaps = pack ? static_cast<int>(std::ceil((numTextures - 1) / 4.f)) : (numTextures - 1);

        int channels = pack ? 4 : 1;

        // Second iteration - create and fill in the blend maps
        const int blendmapSize = (realTextureSize-1) * chunkSize + 1;

        for (int i=0; i<numBlendmaps; ++i)
        {
            GLenum format = pack ? GL_RGBA : GL_ALPHA;

            osg::ref_ptr<osg::Image> image (new osg::Image);
            image->allocateImage(blendmapSize, blendmapSize, 1, format, GL_UNSIGNED_BYTE);
            unsigned char* pData = image->data();

            for (int y=0; y<blendmapSize; ++y)
            {
                for (int x=0; x<blendmapSize; ++x)
                {
                    UniqueTextureId id = getVtexIndexAt(cellX, cellY, x+rowStart, y+colStart);
                    assert(textureIndicesMap.find(id) != textureIndicesMap.end());
                    int layerIndex = textureIndicesMap.find(id)->second;
                    int blendIndex = (pack ? static_cast<int>(std::floor((layerIndex - 1) / 4.f)) : layerIndex - 1);
                    int channel = pack ? std::max(0, (layerIndex-1) % 4) : 0;

                    if (blendIndex == i)
                        pData[y*blendmapSize*channels + x*channels + channel] = 255;
                    else
                        pData[y*blendmapSize*channels + x*channels + channel] = 0;
                }
            }

            blendmaps.push_back(image);
        }
    }

    float Storage::getHeightAt(const osg::Vec3f &worldPos)
    {
        int cellX = static_cast<int>(std::floor(worldPos.x() / 8192.f));
        int cellY = static_cast<int>(std::floor(worldPos.y() / 8192.f));

        const ESM::Land* land = getLand(cellX, cellY);
        if (!land || !(land->mDataTypes&ESM::Land::DATA_VHGT))
            return -2048;

        // Mostly lifted from Ogre::Terrain::getHeightAtTerrainPosition

        // Normalized position in the cell
        float nX = (worldPos.x() - (cellX * 8192))/8192.f;
        float nY = (worldPos.y() - (cellY * 8192))/8192.f;

        // get left / bottom points (rounded down)
        float factor = ESM::Land::LAND_SIZE - 1.0f;
        float invFactor = 1.0f / factor;

        int startX = static_cast<int>(nX * factor);
        int startY = static_cast<int>(nY * factor);
        int endX = startX + 1;
        int endY = startY + 1;

        endX = std::min(endX, ESM::Land::LAND_SIZE-1);
        endY = std::min(endY, ESM::Land::LAND_SIZE-1);

        // now get points in terrain space (effectively rounding them to boundaries)
        float startXTS = startX * invFactor;
        float startYTS = startY * invFactor;
        float endXTS = endX * invFactor;
        float endYTS = endY * invFactor;

        // get parametric from start coord to next point
        float xParam = (nX - startXTS) * factor;
        float yParam = (nY - startYTS) * factor;

        /* For even / odd tri strip rows, triangles are this shape:
        even     odd
        3---2   3---2
        | / |   | \ |
        0---1   0---1
        */

        // Build all 4 positions in normalized cell space, using point-sampled height
        osg::Vec3f v0 (startXTS, startYTS, getVertexHeight(land, startX, startY) / 8192.f);
        osg::Vec3f v1 (endXTS, startYTS, getVertexHeight(land, endX, startY) / 8192.f);
        osg::Vec3f v2 (endXTS, endYTS, getVertexHeight(land, endX, endY) / 8192.f);
        osg::Vec3f v3 (startXTS, endYTS, getVertexHeight(land, startX, endY) / 8192.f);
        // define this plane in terrain space
        osg::Plane plane;
        // FIXME: deal with differing triangle alignment
        if (true)
        {
            // odd row
            bool secondTri = ((1.0 - yParam) > xParam);
            if (secondTri)
                plane = osg::Plane(v0, v1, v3);
            else
                plane = osg::Plane(v1, v2, v3);
        }
        /*
        else
        {
            // even row
            bool secondTri = (yParam > xParam);
            if (secondTri)
                plane.redefine(v0, v2, v3);
            else
                plane.redefine(v0, v1, v2);
        }
        */

        // Solve plane equation for z
        return (-plane.getNormal().x() * nX
                -plane.getNormal().y() * nY
                - plane[3]) / plane.getNormal().z() * 8192;

    }

    float Storage::getVertexHeight(const ESM::Land *land, int x, int y)
    {
        assert(x < ESM::Land::LAND_SIZE);
        assert(y < ESM::Land::LAND_SIZE);
        return land->getLandData()->mHeights[y * ESM::Land::LAND_SIZE + x];
    }

    Terrain::LayerInfo Storage::getLayerInfo(const std::string& texture)
    {
        // Already have this cached?
        std::map<std::string, Terrain::LayerInfo>::iterator found = mLayerInfoMap.find(texture);
        if (found != mLayerInfoMap.end())
            return found->second;

        Terrain::LayerInfo info;
        info.mParallax = false;
        info.mSpecular = false;
        info.mDiffuseMap = texture;
        std::string texture_ = texture;
        boost::replace_last(texture_, ".", "_nh.");

        if (mVFS->exists(texture_))
        {
            info.mNormalMap = texture_;
            info.mParallax = true;
        }
        else
        {
            texture_ = texture;
            boost::replace_last(texture_, ".", "_n.");
            if (mVFS->exists(texture_))
                info.mNormalMap = texture_;
        }

        texture_ = texture;
        boost::replace_last(texture_, ".", "_diffusespec.");
        if (mVFS->exists(texture_))
        {
            info.mDiffuseMap = texture_;
            info.mSpecular = true;
        }

        mLayerInfoMap[texture] = info;

        return info;
    }

    Terrain::LayerInfo Storage::getDefaultLayer()
    {
        Terrain::LayerInfo info;
        info.mDiffuseMap = "textures\\_land_default.dds";
        info.mParallax = false;
        info.mSpecular = false;
        return info;
    }

    float Storage::getCellWorldSize()
    {
        return static_cast<float>(ESM::Land::REAL_SIZE);
    }

    int Storage::getCellVertices()
    {
        return ESM::Land::LAND_SIZE;
    }

}
