#include "storage.hpp"

#include <OgreVector2.h>
#include <OgreTextureManager.h>
#include <OgreStringConverter.h>
#include <OgreRenderSystem.h>
#include <OgreRoot.h>

#include <boost/multi_array.hpp>

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
        if (land && land->mHasData)
        {
            normal.x = land->mLandData->mNormals[col*ESM::Land::LAND_SIZE*3+row*3];
            normal.y = land->mLandData->mNormals[col*ESM::Land::LAND_SIZE*3+row*3+1];
            normal.z = land->mLandData->mNormals[col*ESM::Land::LAND_SIZE*3+row*3+2];
        }
    }

    void Storage::fillVertexBuffers (int lodLevel, float size, const Ogre::Vector2& center,
                            Ogre::HardwareVertexBufferSharedPtr vertexBuffer,
                            Ogre::HardwareVertexBufferSharedPtr normalBuffer,
                            Ogre::HardwareVertexBufferSharedPtr colourBuffer)
    {
        // LOD level n means every 2^n-th vertex is kept
        size_t increment = std::pow(2, lodLevel);

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
                            // Normals don't connect seamlessly between cells - wtf?
                            if (col == ESM::Land::LAND_SIZE-1 || row == ESM::Land::LAND_SIZE-1)
                                fixNormal(normal, cellX, cellY, col, row);
                            // z < 0 should never happen, but it does - I hate this data set...
                            if (normal.z < 0)
                                normal *= -1;
                            normal.normalise();
                        }
                        else
                            normal = Ogre::Vector3(0,0,1);

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
        // If we're at the last row (or last column), we need to get the texture from the neighbour cell
        // to get consistent blending at the border
        if (x >= ESM::Land::LAND_TEXTURE_SIZE)
        {
            cellX++;
            x -= ESM::Land::LAND_TEXTURE_SIZE;
        }
        if (y >= ESM::Land::LAND_TEXTURE_SIZE)
        {
            cellY++;
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
            return "_land_default.dds"; // Not sure if the default texture really is hardcoded?

        // NB: All vtex ids are +1 compared to the ltex ids
        const ESM::LandTexture* ltex = getLandTexture(id.first-1, id.second);

        std::string texture = ltex->mTexture;
        //TODO this is needed due to MWs messed up texture handling
        texture = texture.substr(0, texture.rfind(".")) + ".dds";

        return texture;
    }

    void Storage::getBlendmaps(float chunkSize, const Ogre::Vector2 &chunkCenter,
        bool pack, std::vector<Ogre::TexturePtr> &blendmaps, std::vector<std::string> &layerList)
    {
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
        // NB +1 to get the last index from neighbour cell (see getVtexIndexAt)
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
            layerList.push_back(getTextureName(*it));
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


}
