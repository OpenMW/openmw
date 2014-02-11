#include "storage.hpp"

#include <OgreVector2.h>
#include <OgreTextureManager.h>
#include <OgreStringConverter.h>
#include <OgreRenderSystem.h>
#include <OgreResourceGroupManager.h>
#include <OgreRoot.h>

#include <boost/algorithm/string.hpp>

namespace Terrain
{

    struct VertexElement
    {
        Ogre::Vector3 pos;
        Ogre::Vector3 normal;
        Ogre::ColourValue colour;
    };

    bool Storage::getMinMaxHeights(float size, const Ogre::Vector2 &center, float &min, float &max)
    {
        assert (size <= 1 && "Storage::getMinMaxHeights, chunk size should be <= 1 cell");

        /// \todo investigate if min/max heights should be stored at load time in ESM::Land instead

        Ogre::Vector2 origin = center - Ogre::Vector2(size/2.f, size/2.f);

        assert(origin.x == (int) origin.x);
        assert(origin.y == (int) origin.y);

        int cellX = origin.x;
        int cellY = origin.y;

        const ESM::Land* land = getLand(cellX, cellY);
        if (!land)
            return false;

        min = std::numeric_limits<float>().max();
        max = -std::numeric_limits<float>().max();
        for (int row=0; row<ESM::Land::LAND_SIZE; ++row)
        {
            for (int col=0; col<ESM::Land::LAND_SIZE; ++col)
            {
                float h = land->mLandData->mHeights[col*ESM::Land::LAND_SIZE+row];
                if (h > max)
                    max = h;
                if (h < min)
                    min = h;
            }
        }
        return true;
    }

    void Storage::fixNormal (Ogre::Vector3& normal, int cellX, int cellY, int col, int row)
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
        ESM::Land* land = getLand(cellX, cellY);
        if (land && land->mHasData)
        {
            normal.x = land->mLandData->mNormals[col*ESM::Land::LAND_SIZE*3+row*3];
            normal.y = land->mLandData->mNormals[col*ESM::Land::LAND_SIZE*3+row*3+1];
            normal.z = land->mLandData->mNormals[col*ESM::Land::LAND_SIZE*3+row*3+2];
            normal.normalise();
        }
        else
            normal = Ogre::Vector3(0,0,1);
    }

    void Storage::averageNormal(Ogre::Vector3 &normal, int cellX, int cellY, int col, int row)
    {
        Ogre::Vector3 n1,n2,n3,n4;
        fixNormal(n1, cellX, cellY, col+1, row);
        fixNormal(n2, cellX, cellY, col-1, row);
        fixNormal(n3, cellX, cellY, col, row+1);
        fixNormal(n4, cellX, cellY, col, row-1);
        normal = (n1+n2+n3+n4);
        normal.normalise();
    }

    void Storage::fixColour (Ogre::ColourValue& color, int cellX, int cellY, int col, int row)
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
        ESM::Land* land = getLand(cellX, cellY);
        if (land && land->mLandData->mUsingColours)
        {
            color.r = land->mLandData->mColours[col*ESM::Land::LAND_SIZE*3+row*3] / 255.f;
            color.g = land->mLandData->mColours[col*ESM::Land::LAND_SIZE*3+row*3+1] / 255.f;
            color.b = land->mLandData->mColours[col*ESM::Land::LAND_SIZE*3+row*3+2] / 255.f;
        }
        else
        {
            color.r = 1;
            color.g = 1;
            color.b = 1;
        }

    }

    void Storage::fillVertexBuffers (int lodLevel, float size, const Ogre::Vector2& center,
                            Ogre::HardwareVertexBufferSharedPtr vertexBuffer,
                            Ogre::HardwareVertexBufferSharedPtr normalBuffer,
                            Ogre::HardwareVertexBufferSharedPtr colourBuffer)
    {
        // LOD level n means every 2^n-th vertex is kept
        size_t increment = 1 << lodLevel;

        Ogre::Vector2 origin = center - Ogre::Vector2(size/2.f, size/2.f);
        assert(origin.x == (int) origin.x);
        assert(origin.y == (int) origin.y);

        int startX = origin.x;
        int startY = origin.y;

        size_t numVerts = size*(ESM::Land::LAND_SIZE-1)/increment + 1;

        std::vector<uint8_t> colors;
        colors.resize(numVerts*numVerts*4);
        std::vector<float> positions;
        positions.resize(numVerts*numVerts*3);
        std::vector<float> normals;
        normals.resize(numVerts*numVerts*3);

        Ogre::Vector3 normal;
        Ogre::ColourValue color;

        float vertY;
        float vertX;

        float vertY_ = 0; // of current cell corner
        for (int cellY = startY; cellY < startY + std::ceil(size); ++cellY)
        {
            float vertX_ = 0; // of current cell corner
            for (int cellX = startX; cellX < startX + std::ceil(size); ++cellX)
            {
                ESM::Land* land = getLand(cellX, cellY);
                if (land && !land->mHasData)
                    land = NULL;
                bool hasColors = land && land->mLandData->mUsingColours;

                int rowStart = 0;
                int colStart = 0;
                // Skip the first row / column unless we're at a chunk edge,
                // since this row / column is already contained in a previous cell
                if (colStart == 0 && vertY_ != 0)
                    colStart += increment;
                if (rowStart == 0 && vertX_ != 0)
                    rowStart += increment;

                vertY = vertY_;
                for (int col=colStart; col<ESM::Land::LAND_SIZE; col += increment)
                {
                    vertX = vertX_;
                    for (int row=rowStart; row<ESM::Land::LAND_SIZE; row += increment)
                    {
                        positions[vertX*numVerts*3 + vertY*3] = ((vertX/float(numVerts-1)-0.5) * size * 8192);
                        positions[vertX*numVerts*3 + vertY*3 + 1] = ((vertY/float(numVerts-1)-0.5) * size * 8192);
                        if (land)
                            positions[vertX*numVerts*3 + vertY*3 + 2] = land->mLandData->mHeights[col*ESM::Land::LAND_SIZE+row];
                        else
                            positions[vertX*numVerts*3 + vertY*3 + 2] = -2048;

                        if (land)
                        {
                            normal.x = land->mLandData->mNormals[col*ESM::Land::LAND_SIZE*3+row*3];
                            normal.y = land->mLandData->mNormals[col*ESM::Land::LAND_SIZE*3+row*3+1];
                            normal.z = land->mLandData->mNormals[col*ESM::Land::LAND_SIZE*3+row*3+2];
                            normal.normalise();
                        }
                        else
                            normal = Ogre::Vector3(0,0,1);

                        // Normals apparently don't connect seamlessly between cells
                        if (col == ESM::Land::LAND_SIZE-1 || row == ESM::Land::LAND_SIZE-1)
                            fixNormal(normal, cellX, cellY, col, row);

                        // some corner normals appear to be complete garbage (z < 0)
                        if ((row == 0 || row == ESM::Land::LAND_SIZE-1) && (col == 0 || col == ESM::Land::LAND_SIZE-1))
                            averageNormal(normal, cellX, cellY, col, row);

                        assert(normal.z > 0);

                        normals[vertX*numVerts*3 + vertY*3] = normal.x;
                        normals[vertX*numVerts*3 + vertY*3 + 1] = normal.y;
                        normals[vertX*numVerts*3 + vertY*3 + 2] = normal.z;

                        if (hasColors)
                        {
                            color.r = land->mLandData->mColours[col*ESM::Land::LAND_SIZE*3+row*3] / 255.f;
                            color.g = land->mLandData->mColours[col*ESM::Land::LAND_SIZE*3+row*3+1] / 255.f;
                            color.b = land->mLandData->mColours[col*ESM::Land::LAND_SIZE*3+row*3+2] / 255.f;
                        }
                        else
                        {
                            color.r = 1;
                            color.g = 1;
                            color.b = 1;
                        }

                        // Unlike normals, colors mostly connect seamlessly between cells, but not always...
                        if (col == ESM::Land::LAND_SIZE-1 || row == ESM::Land::LAND_SIZE-1)
                            fixColour(color, cellX, cellY, col, row);

                        color.a = 1;
                        Ogre::uint32 rsColor;
                        Ogre::Root::getSingleton().getRenderSystem()->convertColourValue(color, &rsColor);
                        memcpy(&colors[vertX*numVerts*4 + vertY*4], &rsColor, sizeof(Ogre::uint32));

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

        vertexBuffer->writeData(0, vertexBuffer->getSizeInBytes(), &positions[0], true);
        normalBuffer->writeData(0, normalBuffer->getSizeInBytes(), &normals[0], true);
        colourBuffer->writeData(0, colourBuffer->getSizeInBytes(), &colors[0], true);
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

        ESM::Land* land = getLand(cellX, cellY);
        if (land)
        {
            if (!land->isDataLoaded(ESM::Land::DATA_VTEX))
                land->loadData(ESM::Land::DATA_VTEX);

            int tex = land->mLandData->mTextures[y * ESM::Land::LAND_TEXTURE_SIZE + x];
            if (tex == 0)
                return std::make_pair(0,0); // vtex 0 is always the base texture, regardless of plugin
            return std::make_pair(tex, land->mPlugin);
        }
        else
            return std::make_pair(0,0);
    }

    std::string Storage::getTextureName(UniqueTextureId id)
    {
        if (id.first == 0)
            return "_land_default.dds"; // Not sure if the default texture floatly is hardcoded?

        // NB: All vtex ids are +1 compared to the ltex ids
        const ESM::LandTexture* ltex = getLandTexture(id.first-1, id.second);

        std::string texture = ltex->mTexture;
        //TODO this is needed due to MWs messed up texture handling
        texture = texture.substr(0, texture.rfind(".")) + ".dds";

        return texture;
    }

    void Storage::getBlendmaps(float chunkSize, const Ogre::Vector2 &chunkCenter,
        bool pack, std::vector<Ogre::TexturePtr> &blendmaps, std::vector<LayerInfo> &layerList)
    {
        // TODO - blending isn't completely right yet; the blending radius appears to be
        // different at a cell transition (2 vertices, not 4), so we may need to create a larger blendmap
        // and interpolate the rest of the cell by hand? :/

        Ogre::Vector2 origin = chunkCenter - Ogre::Vector2(chunkSize/2.f, chunkSize/2.f);
        int cellX = origin.x;
        int cellY = origin.y;

        // Save the used texture indices so we know the total number of textures
        // and number of required blend maps
        std::set<UniqueTextureId> textureIndices;
        // Due to the way the blending works, the base layer will always shine through in between
        // blend transitions (eg halfway between two texels, both blend values will be 0.5, so 25% of base layer visible).
        // To get a consistent look, we need to make sure to use the same base layer in all cells.
        // So we're always adding _land_default.dds as the base layer here, even if it's not referenced in this cell.
        textureIndices.insert(std::make_pair(0,0));

        for (int y=0; y<ESM::Land::LAND_TEXTURE_SIZE+1; ++y)
            for (int x=0; x<ESM::Land::LAND_TEXTURE_SIZE+1; ++x)
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
        int numBlendmaps = pack ? std::ceil((numTextures-1) / 4.f) : (numTextures-1);

        int channels = pack ? 4 : 1;

        // Second iteration - create and fill in the blend maps
        const int blendmapSize = ESM::Land::LAND_TEXTURE_SIZE+1;
        std::vector<Ogre::uchar> data;
        data.resize(blendmapSize * blendmapSize * channels, 0);

        for (int i=0; i<numBlendmaps; ++i)
        {
            Ogre::PixelFormat format = pack ? Ogre::PF_A8B8G8R8 : Ogre::PF_A8;
            static int count=0;
            Ogre::TexturePtr map = Ogre::TextureManager::getSingleton().createManual("terrain/blend/"
                + Ogre::StringConverter::toString(count++), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
               Ogre::TEX_TYPE_2D, blendmapSize, blendmapSize, 0, format);

            for (int y=0; y<blendmapSize; ++y)
            {
                for (int x=0; x<blendmapSize; ++x)
                {
                    UniqueTextureId id = getVtexIndexAt(cellX, cellY, x, y);
                    int layerIndex = textureIndicesMap.find(id)->second;
                    int blendIndex = (pack ? std::floor((layerIndex-1)/4.f) : layerIndex-1);
                    int channel = pack ? std::max(0, (layerIndex-1) % 4) : 0;

                    if (blendIndex == i)
                        data[y*blendmapSize*channels + x*channels + channel] = 255;
                    else
                        data[y*blendmapSize*channels + x*channels + channel] = 0;
                }
            }

            // All done, upload to GPU
            Ogre::DataStreamPtr stream(new Ogre::MemoryDataStream(&data[0], data.size()));
            map->loadRawData(stream, blendmapSize, blendmapSize, format);
            blendmaps.push_back(map);
        }
    }

    float Storage::getHeightAt(const Ogre::Vector3 &worldPos)
    {
        int cellX = std::floor(worldPos.x / 8192.f);
        int cellY = std::floor(worldPos.y / 8192.f);

        ESM::Land* land = getLand(cellX, cellY);
        if (!land)
            return -2048;

        // Mostly lifted from Ogre::Terrain::getHeightAtTerrainPosition

        // Normalized position in the cell
        float nX = (worldPos.x - (cellX * 8192))/8192.f;
        float nY = (worldPos.y - (cellY * 8192))/8192.f;

        // get left / bottom points (rounded down)
        float factor = ESM::Land::LAND_SIZE - 1.0f;
        float invFactor = 1.0f / factor;

        int startX = static_cast<int>(nX * factor);
        int startY = static_cast<int>(nY * factor);
        int endX = startX + 1;
        int endY = startY + 1;

        assert(endX < ESM::Land::LAND_SIZE);
        assert(endY < ESM::Land::LAND_SIZE);

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
        Ogre::Vector3 v0 (startXTS, startYTS, getVertexHeight(land, startX, startY) / 8192.f);
        Ogre::Vector3 v1 (endXTS, startYTS, getVertexHeight(land, endX, startY) / 8192.f);
        Ogre::Vector3 v2 (endXTS, endYTS, getVertexHeight(land, endX, endY) / 8192.f);
        Ogre::Vector3 v3 (startXTS, endYTS, getVertexHeight(land, startX, endY) / 8192.f);
        // define this plane in terrain space
        Ogre::Plane plane;
        // (At the moment, all rows have the same triangle alignment)
        if (true)
        {
            // odd row
            bool secondTri = ((1.0 - yParam) > xParam);
            if (secondTri)
                plane.redefine(v0, v1, v3);
            else
                plane.redefine(v1, v2, v3);
        }
        else
        {
            // even row
            bool secondTri = (yParam > xParam);
            if (secondTri)
                plane.redefine(v0, v2, v3);
            else
                plane.redefine(v0, v1, v2);
        }

        // Solve plane equation for z
        return (-plane.normal.x * nX
                -plane.normal.y * nY
                - plane.d) / plane.normal.z * 8192;

    }

    float Storage::getVertexHeight(const ESM::Land *land, int x, int y)
    {
        assert(x < ESM::Land::LAND_SIZE);
        assert(y < ESM::Land::LAND_SIZE);
        return land->mLandData->mHeights[y * ESM::Land::LAND_SIZE + x];
    }

    LayerInfo Storage::getLayerInfo(const std::string& texture)
    {
        // Already have this cached?
        if (mLayerInfoMap.find(texture) != mLayerInfoMap.end())
            return mLayerInfoMap[texture];

        LayerInfo info;
        info.mParallax = false;
        info.mSpecular = false;
        info.mDiffuseMap = "textures\\" + texture;
        std::string texture_ = texture;
        boost::replace_last(texture_, ".", "_nh.");
        if (Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup("textures\\" + texture_))
        {
            info.mNormalMap = "textures\\" + texture_;
            info.mParallax = true;
        }
        else
        {
            texture_ = texture;
            boost::replace_last(texture_, ".", "_n.");
            if (Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup("textures\\" + texture_))
                info.mNormalMap = "textures\\" + texture_;
        }

        texture_ = texture;
        boost::replace_last(texture_, ".", "_diffusespec.");
        if (Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup("textures\\" + texture_))
        {
            info.mDiffuseMap = "textures\\" + texture_;
            info.mSpecular = true;
        }

        mLayerInfoMap[texture] = info;

        return info;
    }


}
