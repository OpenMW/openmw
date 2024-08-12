#include "storage.hpp"

#include <algorithm>
#include <optional>
#include <stdexcept>

#include <osg/Image>
#include <osg/Plane>

#include <components/debug/debuglog.hpp>
#include <components/esm/esmterrain.hpp>
#include <components/esm/util.hpp>
#include <components/esm3/loadland.hpp>
#include <components/esm4/loadland.hpp>
#include <components/esm4/loadltex.hpp>
#include <components/esm4/loadtxst.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/misc/strings/algorithm.hpp>
#include <components/vfs/manager.hpp>

#include "gridsampling.hpp"

namespace ESMTerrain
{
    namespace
    {
        UniqueTextureId getTextureIdAt(const LandObject* land, std::size_t x, std::size_t y)
        {
            assert(x < ESM::Land::LAND_TEXTURE_SIZE);
            assert(y < ESM::Land::LAND_TEXTURE_SIZE);

            if (land == nullptr)
                return { 0, 0 };

            const ESM::LandData* data = land->getData(ESM::Land::DATA_VTEX);
            if (data == nullptr)
                return { 0, 0 };

            const std::uint16_t tex = data->getTextures()[y * ESM::Land::LAND_TEXTURE_SIZE + x];
            if (tex == 0)
                return { 0, 0 }; // vtex 0 is always the base texture, regardless of plugin

            return { tex, land->getPlugin() };
        }
#if 0
        UniqueTextureId getQuadTextureIdAt(const ESM4::Land* land, std::size_t x, std::size_t y)
        {
            assert(x < 17);
            assert(y < 17);

            if (land == nullptr)
                return { 0, 0 };

            const ESM::LandData* data = land->getData(ESM::Land::DATA_VTEX);
            if (data == nullptr)
                return { 0, 0 };

            const std::uint16_t tex = data->getTextures()[y * ESM::Land::LAND_TEXTURE_SIZE + x];
            if (tex == 0)
                return { 0, 0 }; // vtex 0 is always the base texture, regardless of plugin

            return { tex, land->getPlugin() };
        }
#endif
    }

    class LandCache
    {
    public:
        explicit LandCache(int offsetX, int offsetY, std::size_t size)
            : mOffsetX(offsetX)
            , mOffsetY(offsetY)
            , mSize(size)
            , mValues(size * size)
        {
        }

        std::optional<const LandObject*> find(int x, int y) const
        {
            const std::size_t index = getIndex(x, y);
            if (const auto& value = mValues[index])
                return value->get();
            return std::nullopt;
        }

        void insert(int x, int y, osg::ref_ptr<const LandObject>&& value)
        {
            const std::size_t index = getIndex(x, y);
            mValues[index] = std::move(value);
        }

    private:
        int mOffsetX;
        int mOffsetY;
        std::size_t mSize;
        std::vector<std::optional<osg::ref_ptr<const LandObject>>> mValues;

        std::size_t getIndex(int x, int y) const
        {
            return normalizeCoordinate(x, mOffsetX) * mSize + normalizeCoordinate(y, mOffsetY);
        }

        std::size_t normalizeCoordinate(int value, int offset) const
        {
            assert(value >= offset);
            assert(value < offset + static_cast<int>(mSize));
            return static_cast<std::size_t>(value - offset);
        }
    };

    LandObject::LandObject(const ESM4::Land& land, int loadFlags)
        : mData(land, loadFlags)
    {
    }

    LandObject::LandObject(const ESM::Land& land, int loadFlags)
        : mData(land, loadFlags)
    {
    }

    LandObject::LandObject(const LandObject& /*copy*/, const osg::CopyOp& /*copyOp*/)
    {
        throw std::logic_error("LandObject copy constructor is not implemented");
    }

    const float defaultHeight = ESM::Land::DEFAULT_HEIGHT;

    Storage::Storage(const VFS::Manager* vfs, std::string_view normalMapPattern,
        std::string_view normalHeightMapPattern, bool autoUseNormalMaps, std::string_view specularMapPattern,
        bool autoUseSpecularMaps)
        : mIsEsm4Ext(false)
        , mVFS(vfs)
        , mNormalMapPattern(normalMapPattern)
        , mNormalHeightMapPattern(normalHeightMapPattern)
        , mAutoUseNormalMaps(autoUseNormalMaps)
        , mSpecularMapPattern(specularMapPattern)
        , mAutoUseSpecularMaps(autoUseSpecularMaps)
    {
    }

    bool Storage::getMinMaxHeights(float size, const osg::Vec2f& center, ESM::RefId worldspace, float& min, float& max)
    {
        assert(size <= 1 && "Storage::getMinMaxHeights, chunk size should be <= 1 cell");

        osg::Vec2f origin = center - osg::Vec2f(size / 2.f, size / 2.f);

        int cellX = static_cast<int>(std::floor(origin.x()));
        int cellY = static_cast<int>(std::floor(origin.y()));
        osg::ref_ptr<const LandObject> land = getLand(ESM::ExteriorCellLocation(cellX, cellY, worldspace));
        const ESM::LandData* data = land ? land->getData(ESM::Land::DATA_VHGT) : nullptr;
        const int landSize = ESM::getLandSize(worldspace);
        int startRow = (origin.x() - cellX) * landSize;
        int startColumn = (origin.y() - cellY) * landSize;

        int endRow = startRow + size * (landSize - 1) + 1;
        int endColumn = startColumn + size * (landSize - 1) + 1;

        if (data)
        {
            min = std::numeric_limits<float>::max();
            max = -std::numeric_limits<float>::max();
            for (int row = startRow; row < endRow; ++row)
            {
                for (int col = startColumn; col < endColumn; ++col)
                {
                    float h = data->getHeights()[col * landSize + row];
                    if (h > max)
                        max = h;
                    if (h < min)
                        min = h;
                }
            }
            return true;
        }

        min = defaultHeight;
        max = defaultHeight;
        return false;
    }

    void Storage::fixNormal(
        osg::Vec3f& normal, ESM::ExteriorCellLocation cellLocation, int col, int row, LandCache& cache)
    {

        const int landSize = ESM::getLandSize(cellLocation.mWorldspace);

        while (col >= landSize - 1)
        {
            ++cellLocation.mY;
            col -= landSize - 1;
        }
        while (row >= landSize - 1)
        {
            ++cellLocation.mX;
            row -= landSize - 1;
        }
        while (col < 0)
        {
            --cellLocation.mY;
            col += landSize - 1;
        }
        while (row < 0)
        {
            --cellLocation.mX;
            row += landSize - 1;
        }
        const LandObject* land = getLand(cellLocation, cache);
        const ESM::LandData* data = land ? land->getData(ESM::Land::DATA_VNML) : nullptr;
        if (data)
        {
            normal.x() = data->getNormals()[col * landSize * 3 + row * 3];
            normal.y() = data->getNormals()[col * landSize * 3 + row * 3 + 1];
            normal.z() = data->getNormals()[col * landSize * 3 + row * 3 + 2];
            normal.normalize();
        }
        else
            normal = osg::Vec3f(0, 0, 1);
    }

    void Storage::averageNormal(
        osg::Vec3f& normal, ESM::ExteriorCellLocation cellLocation, int col, int row, LandCache& cache)
    {
        osg::Vec3f n1, n2, n3, n4;
        fixNormal(n1, cellLocation, col + 1, row, cache);
        fixNormal(n2, cellLocation, col - 1, row, cache);
        fixNormal(n3, cellLocation, col, row + 1, cache);
        fixNormal(n4, cellLocation, col, row - 1, cache);
        normal = (n1 + n2 + n3 + n4);
        normal.normalize();
    }

    void Storage::fixColour(
        osg::Vec4ub& color, ESM::ExteriorCellLocation cellLocation, int col, int row, LandCache& cache)
    {

        const int landSize = ESM::getLandSize(cellLocation.mWorldspace);

        if (col == landSize - 1)
        {
            ++cellLocation.mY;
            col = 0;
        }
        if (row == landSize - 1)
        {
            ++cellLocation.mX;
            row = 0;
        }
        const LandObject* land = getLand(cellLocation, cache);
        const ESM::LandData* data = land ? land->getData(ESM::Land::DATA_VCLR) : nullptr;
        if (data)
        {
            color.r() = data->getColors()[col * landSize * 3 + row * 3];
            color.g() = data->getColors()[col * landSize * 3 + row * 3 + 1];
            color.b() = data->getColors()[col * landSize * 3 + row * 3 + 2];
        }
        else
        {
            color.r() = 255;
            color.g() = 255;
            color.b() = 255;
        }
    }

    void Storage::fillVertexBuffers(int lodLevel, float size, const osg::Vec2f& center, ESM::RefId worldspace,
        osg::Vec3Array& positions, osg::Vec3Array& normals, osg::Vec4ubArray& colours)
    {
        if (lodLevel < 0 || 63 < lodLevel)
            throw std::invalid_argument("Invalid terrain lod level: " + std::to_string(lodLevel));

        if (size <= 0)
            throw std::invalid_argument("Invalid terrain size: " + std::to_string(size));

        // LOD level n means every 2^n-th vertex is kept
        const std::size_t sampleSize = std::size_t{ 1 } << lodLevel;
        const std::size_t cellSize = static_cast<std::size_t>(ESM::getLandSize(worldspace));
        const std::size_t numVerts = static_cast<std::size_t>(size * (cellSize - 1) / sampleSize) + 1;

        positions.resize(numVerts * numVerts);
        normals.resize(numVerts * numVerts);
        colours.resize(numVerts * numVerts);

        const bool alteration = useAlteration();
        const int landSizeInUnits = ESM::getCellSize(worldspace);
        const osg::Vec2f origin = center - osg::Vec2f(size, size) * 0.5f;
        const int startCellX = static_cast<int>(std::floor(origin.x()));
        const int startCellY = static_cast<int>(std::floor(origin.y()));
        LandCache cache(startCellX - 1, startCellY - 1, static_cast<std::size_t>(std::ceil(size)) + 2);
        std::pair lastCell{ startCellX, startCellY };
        const LandObject* land = getLand(ESM::ExteriorCellLocation(startCellX, startCellY, worldspace), cache);
        const ESM::LandData* heightData = nullptr;
        const ESM::LandData* normalData = nullptr;
        const ESM::LandData* colourData = nullptr;
        bool validHeightDataExists = false;

        if (land != nullptr)
        {
            heightData = land->getData(ESM::Land::DATA_VHGT);
            normalData = land->getData(ESM::Land::DATA_VNML);
            colourData = land->getData(ESM::Land::DATA_VCLR);
            validHeightDataExists = true;
        }

        const auto handleSample = [&](std::size_t cellShiftX, std::size_t cellShiftY, std::size_t row, std::size_t col,
                                      std::size_t vertX, std::size_t vertY) {
            const int cellX = startCellX + cellShiftX;
            const int cellY = startCellY + cellShiftY;
            const std::pair cell{ cellX, cellY };
            const ESM::ExteriorCellLocation cellLocation(cellX, cellY, worldspace);

            if (lastCell != cell)
            {
                land = getLand(cellLocation, cache);

                heightData = nullptr;
                normalData = nullptr;
                colourData = nullptr;

                if (land != nullptr)
                {
                    heightData = land->getData(ESM::Land::DATA_VHGT);
                    normalData = land->getData(ESM::Land::DATA_VNML);
                    colourData = land->getData(ESM::Land::DATA_VCLR);
                    validHeightDataExists = true;
                }

                lastCell = cell;
            }

            float height = defaultHeight;
            if (heightData != nullptr)
                height = heightData->getHeights()[col * cellSize + row];
            if (alteration)
                height += getAlteredHeight(col, row);

            const std::size_t vertIndex = vertX * numVerts + vertY;

            positions[vertIndex]
                = osg::Vec3f((vertX / static_cast<float>(numVerts - 1) - 0.5f) * size * landSizeInUnits,
                    (vertY / static_cast<float>(numVerts - 1) - 0.5f) * size * landSizeInUnits, height);

            const std::size_t srcArrayIndex = col * cellSize * 3 + row * 3;

            osg::Vec3f normal(0, 0, 1);

            if (normalData != nullptr)
            {
                for (std::size_t i = 0; i < 3; ++i)
                    normal[i] = normalData->getNormals()[srcArrayIndex + i];

                normal.normalize();
            }

            // Normals apparently don't connect seamlessly between cells
            if (col == cellSize - 1 || row == cellSize - 1)
                fixNormal(normal, cellLocation, col, row, cache);

            // some corner normals appear to be complete garbage (z < 0)
            if ((row == 0 || row == cellSize - 1) && (col == 0 || col == cellSize - 1))
                averageNormal(normal, cellLocation, col, row, cache);

            assert(normal.z() > 0);

            normals[vertIndex] = normal;

            osg::Vec4ub color(255, 255, 255, 255);

            if (colourData != nullptr)
                for (std::size_t i = 0; i < 3; ++i)
                    color[i] = colourData->getColors()[srcArrayIndex + i];

            // Does nothing by default, override in OpenMW-CS
            if (alteration)
                adjustColor(col, row, heightData, color);

            // Unlike normals, colors mostly connect seamlessly between cells, but not always...
            if (col == cellSize - 1 || row == cellSize - 1)
                fixColour(color, cellLocation, col, row, cache);

            colours[vertIndex] = color;
        };

        const std::size_t beginX = static_cast<std::size_t>((origin.x() - startCellX) * cellSize);
        const std::size_t beginY = static_cast<std::size_t>((origin.y() - startCellY) * cellSize);
        const std::size_t distance = static_cast<std::size_t>(size * (cellSize - 1)) + 1;

        sampleCellGrid(cellSize, sampleSize, beginX, beginY, distance, handleSample);

        if (!validHeightDataExists && ESM::isEsm4Ext(worldspace))
            std::fill(positions.begin(), positions.end(), osg::Vec3f());
    }

    // NOTE: getLandTexture() is implemented by our child class.  Also note that ESM4 doesn't
    //       want to call correctTexturePath().  We need a way of figuring out that we are in
    //       ESM4 worldspace, either via a parameter or via a state kept as a member to Storage
    //       or TerrainStorage (i.e. our child class).
    std::string Storage::getTextureName(UniqueTextureId id)
    {
        std::string_view texture = "_land_default.dds";
        if (id.first != 0)
        {
            // NB: All vtex ids are +1 compared to the ltex ids
            const std::string* ltex = getLandTexture(id.first - 1, id.second);
            if (ltex)
                texture = *ltex;
            else
            {
                Log(Debug::Warning) << "Warning: Unable to find land texture index " << id.first - 1 << " in plugin "
                                    << id.second << ", using default texture instead";
            }
        }
        // this is needed due to MWs messed up texture handling
        return Misc::ResourceHelpers::correctTexturePath(texture, mVFS);
    }

    // FIXME: for FO3/FONV/TES5 this is rather inefficient since the TextureSet indicates
    //        whether normal map exists, etc, saving us the need to do any searching
    //        in getLayerInfo()
    //
    //        maybe if ltex->mTextureFile is empty simply return a null string and process
    //        differently?
    std::string Storage::getEsm4TextureName(ESM::RefId id)
    {
        //if (mIsEsm4Ext)
        if (const ESM4::LandTexture *ltex = getEsm4LandTexture(id))
        {
            if (ltex->mTextureFile.empty()) // WARN: we assume FO3/FONV/TES5
            {
                if (const ESM4::TextureSet *txst = getEsm4TextureSet(ltex->mTexture))
                {
                    return "textures\\"+txst->mDiffuse;
                }
            }
            else
                return "textures\\landscape\\"+ltex->mTextureFile;
        }

        // FIXME: add a debug log here
        return "";
    }

    // FIXME: May need some changes here to support ESM4 terrain.  Not sure how to deal with many
    //        chunks (i.e. 4 ESM4 quads).  Maybe we just go with the flow here, but do
    //        things 4 times as much?
    //
    //        For now decided to create another method instead (getQuadBlendmaps).
    void Storage::getBlendmaps(float chunkSize, const osg::Vec2f& chunkCenter, ImageVector& blendmaps,
        std::vector<Terrain::LayerInfo>& layerList, ESM::RefId worldspace)
    {
        const osg::Vec2f origin = chunkCenter - osg::Vec2f(chunkSize, chunkSize) * 0.5f;
        const int startCellX = static_cast<int>(std::floor(origin.x()));
        const int startCellY = static_cast<int>(std::floor(origin.y()));
        const std::size_t blendmapSize = getBlendmapSize(chunkSize, ESM::Land::LAND_TEXTURE_SIZE);
        // We need to upscale the blendmap 2x with nearest neighbor sampling to look like Vanilla
        constexpr std::size_t imageScaleFactor = 2;
        const std::size_t blendmapImageSize = blendmapSize * imageScaleFactor;

        std::vector<UniqueTextureId> textureIds(blendmapSize * blendmapSize);
        LandCache cache(startCellX - 1, startCellY - 1, static_cast<std::size_t>(std::ceil(chunkSize)) + 2);
        std::pair lastCell{ startCellX, startCellY };
        const LandObject* land = getLand(ESM::ExteriorCellLocation(startCellX, startCellY, worldspace), cache);

        const auto handleSample = [&](const CellSample& sample) {
            const std::pair cell{ sample.mCellX, sample.mCellY };
            if (lastCell != cell)
            {
                land = getLand(ESM::ExteriorCellLocation(sample.mCellX, sample.mCellY, worldspace), cache);
                lastCell = cell;
            }

            textureIds[sample.mDstCol * blendmapSize + sample.mDstRow]
                = getTextureIdAt(land, sample.mSrcRow, sample.mSrcCol);
        };

        sampleBlendmaps(chunkSize, origin.x(), origin.y(), ESM::Land::LAND_TEXTURE_SIZE, handleSample);

        std::map<UniqueTextureId, std::size_t> textureIndicesMap;

        for (std::size_t y = 0; y < blendmapSize; ++y)
        {
            for (std::size_t x = 0; x < blendmapSize; ++x)
            {
                const UniqueTextureId id = textureIds[y * blendmapSize + x];
                auto found = textureIndicesMap.find(id);
                if (found == textureIndicesMap.end())
                {
                    std::size_t layerIndex = layerList.size();
                    Terrain::LayerInfo info = getLayerInfo(getTextureName(id));

                    // look for existing diffuse map, which may be present when several plugins use the same texture
                    for (std::size_t i = 0; i < layerList.size(); ++i)
                    {
                        if (layerList[i].mDiffuseMap == info.mDiffuseMap)
                        {
                            layerIndex = i;
                            break;
                        }
                    }

                    found = textureIndicesMap.emplace(id, layerIndex).first;

                    if (layerIndex >= layerList.size())
                    {
                        osg::ref_ptr<osg::Image> image(new osg::Image);
                        image->allocateImage(static_cast<int>(blendmapImageSize), static_cast<int>(blendmapImageSize),
                            1, GL_ALPHA, GL_UNSIGNED_BYTE);
                        std::memset(image->data(), 0, image->getTotalDataSize());
                        blendmaps.push_back(std::move(image));
                        layerList.push_back(std::move(info));
                    }
                }
                const std::size_t layerIndex = found->second;
                unsigned char* const data = blendmaps[layerIndex]->data();
                const std::size_t realY = (blendmapSize - y - 1) * imageScaleFactor;
                const std::size_t realX = x * imageScaleFactor;
                data[((realY + 0) * blendmapImageSize + realX + 0)] = 255;
                data[((realY + 1) * blendmapImageSize + realX + 0)] = 255;
                data[((realY + 0) * blendmapImageSize + realX + 1)] = 255;
                data[((realY + 1) * blendmapImageSize + realX + 1)] = 255;
            }
        }

        if (blendmaps.size() == 1)
            blendmaps.clear(); // If a single texture fills the whole terrain, there is no need to blend
    }

    float Storage::getHeightAt(const osg::Vec3f& worldPos, ESM::RefId worldspace)
    {
        const float cellSize = ESM::getCellSize(worldspace);
        int cellX = static_cast<int>(std::floor(worldPos.x() / cellSize));
        int cellY = static_cast<int>(std::floor(worldPos.y() / cellSize));

        osg::ref_ptr<const LandObject> land = getLand(ESM::ExteriorCellLocation(cellX, cellY, worldspace));
        if (!land)
            return ESM::isEsm4Ext(worldspace) ? std::numeric_limits<float>::lowest() : defaultHeight;

        const ESM::LandData* data = land->getData(ESM::Land::DATA_VHGT);
        if (!data)
            return defaultHeight;
        const int landSize = data->getLandSize();

        // Mostly lifted from Ogre::Terrain::getHeightAtTerrainPosition

        // Normalized position in the cell
        float nX = (worldPos.x() - (cellX * cellSize)) / cellSize;
        float nY = (worldPos.y() - (cellY * cellSize)) / cellSize;

        // get left / bottom points (rounded down)
        float factor = landSize - 1.0f;
        float invFactor = 1.0f / factor;

        int startX = static_cast<int>(nX * factor);
        int startY = static_cast<int>(nY * factor);
        int endX = startX + 1;
        int endY = startY + 1;

        endX = std::min(endX, landSize - 1);
        endY = std::min(endY, landSize - 1);

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
        osg::Vec3f v0(startXTS, startYTS, getVertexHeight(data, startX, startY) / cellSize);
        osg::Vec3f v1(endXTS, startYTS, getVertexHeight(data, endX, startY) / cellSize);
        osg::Vec3f v2(endXTS, endYTS, getVertexHeight(data, endX, endY) / cellSize);
        osg::Vec3f v3(startXTS, endYTS, getVertexHeight(data, startX, endY) / cellSize);
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
        return (-plane.getNormal().x() * nX - plane.getNormal().y() * nY - plane[3]) / plane.getNormal().z() * cellSize;
    }

    const LandObject* Storage::getLand(ESM::ExteriorCellLocation cellLocation, LandCache& cache)
    {
        if (const auto land = cache.find(cellLocation.mX, cellLocation.mY))
            return *land;
        osg::ref_ptr<const LandObject> land = getLand(cellLocation);
        const LandObject* result = land.get();
        cache.insert(cellLocation.mX, cellLocation.mY, std::move(land));
        return result;
    }

    void Storage::adjustColor(int col, int row, const ESM::LandData* heightData, osg::Vec4ub& color) const {}

    float Storage::getAlteredHeight(int col, int row) const
    {
        return 0;
    }

    Terrain::LayerInfo Storage::getLayerInfo(const std::string& texture)
    {
        std::lock_guard<std::mutex> lock(mLayerInfoMutex);

        // Already have this cached?
        std::map<std::string, Terrain::LayerInfo>::iterator found = mLayerInfoMap.find(texture);
        if (found != mLayerInfoMap.end())
            return found->second;

        Terrain::LayerInfo info;
        info.mParallax = false;
        info.mSpecular = false;
        //info.mIsEsm4 = false; // hint for Terrain::createPasses()
        info.mDiffuseMap = texture;

        if (mAutoUseNormalMaps)
        {
            std::string texture_ = texture;
            Misc::StringUtils::replaceLast(texture_, ".", mNormalHeightMapPattern + ".");
            if (mVFS->exists(texture_))
            {
                info.mNormalMap = std::move(texture_);
                info.mParallax = true;
            }
            else
            {
                texture_ = texture;
                Misc::StringUtils::replaceLast(texture_, ".", mNormalMapPattern + ".");
                if (mVFS->exists(texture_))
                    info.mNormalMap = std::move(texture_);
            }
        }

        if (mAutoUseSpecularMaps)
        {
            std::string texture_ = texture;
            Misc::StringUtils::replaceLast(texture_, ".", mSpecularMapPattern + ".");
            if (mVFS->exists(texture_))
            {
                info.mDiffuseMap = std::move(texture_);
                info.mSpecular = true;
            }
        }

        mLayerInfoMap[texture] = info;

        return info;
    }

    Terrain::LayerInfo Storage::getLayerInfo(const ESM4::TextureSet *txst)
    {
        Terrain::LayerInfo info;
        info.mDiffuseMap = "";
        info.mNormalMap = "";
        info.mParallax = false;
        info.mSpecular = false;
        //info.mIsEsm4 = true; // hint for Terrain::createPasses()

        if (txst)
        {
            assert(!txst->mDiffuseMap.empty() && "getlayerInfo: empty diffuse map");

            std::string diffuse = "textures\\landscape\\"+txst->mDiffuse;
            std::map<std::string, Terrain::LayerInfo>::iterator found = mLayerInfoMap.find(diffuse);
            if (found != mLayerInfoMap.end())
                return found->second;

            info.mDiffuseMap = diffuse;
            if (!txst->mNormalMap.empty())
                info.mNormalMap = "textures\\landscape\\"+txst->mNormalMap;

            // FIXME: this flag indicates height info in alpha channel of normal map
            //        but the normal map alpha channel has specular info instead
            //        (probably needs some flag in the terrain shader to fix)
            info.mParallax = false;
            // FIXME: this flag indicates specular info in alpha channel of diffuse
            //        but the diffuse alpha channel has transparency data instead
            //        (probably needs some flag in the terrain shader to fix)
            info.mSpecular = false;

            // FIXME: should support other features of ESM4::TextureSet
            //        probably need corresponding support in the terrain shader

            mLayerInfoMap[diffuse] = info;
        }

        return info;
    }

    float Storage::getCellWorldSize(ESM::RefId worldspace)
    {
        return static_cast<float>(ESM::getCellSize(worldspace));
    }

    int Storage::getCellVertices(ESM::RefId worldspace)
    {
        return ESM::getLandSize(worldspace);
    }

    // NOTE: For now we are only conident when chunkSize is 1.  Needs more testing to see
    //       if this will work with different chunkSize values.
    //
    //       This is called by ChunkManager::createPasses() which then calls
    //       Terrain::createPasses() which ultimately calls BlendmapTexMat::value().
    //       I suspect that is where UV mapping is done (just a guess; LayerTexMat
    //       may need to be looked at as well).
    //
    // WARN: the value sQuadTexturePerSide was determined empirically for TES4 only
    //       FO3/FONV/TES5 may well have a different value - needs testing
    int Storage::getBlendmapScale(float chunkSize)
    {
        if (mIsEsm4Ext)
        {
            //std::cout << "blendmap scale "
                //<< std::to_string(ESM4::Land::sQuadTexturePerSide * chunkSize) << std::endl;
            return ESM4::Land::sQuadTexturePerSide;// * chunkSize;
        }

        return ESM::Land::LAND_TEXTURE_SIZE * chunkSize;
    }

    void Storage::fillQuadVertexBuffers(float size, const osg::Vec2f& center, ESM::RefId worldspace,
            osg::Vec3Array& positions, osg::Vec3Array& normals, osg::Vec4ubArray& colours, int quad)
    {
        // sampleSize is not used but declared here in order to keep the code as close to
        // fillVertexBuffers() as possible
        const std::size_t sampleSize = 1;

        // DEBUG NOTES: cellSize should be 33 for ESM4
        //              numVerts should be 17 for ESM4
        const std::size_t cellSize = static_cast<std::size_t>(ESM::getLandSize(worldspace));
        const std::size_t numVerts = static_cast<std::size_t>(size * (cellSize - 1) / sampleSize) + 1;

        positions.resize(numVerts*numVerts*3);
        normals.resize(numVerts*numVerts*3);
        colours.resize(numVerts*numVerts*4);

        const bool alteration = useAlteration(); // Does nothing by default, override in OpenMW-CS
        const int landSizeInUnits = ESM::getCellSize(worldspace);

// I think the current code copied from fillVertexBuffers() works fine
#if 0
        // NOTE: here center is the center of the ESM4 cell (in terms of cell grid position) which
        //       is subtly different to the way fillVertexBuffers() treats it because we don't
        //       worry about chunk sizes or LOD
        //
        //       center is wrong here due to the way TerrainGrid::buildTerrain() calculates the
        //       new center
        osg::Vec2f realCenter;
        switch (quad)
        {
            case 3: realCenter = center - osg::Vec2f( 0.25f,  0.25f); break;
            case 1: realCenter = center - osg::Vec2f( 0.25f, -0.25f); break;
            case 2: realCenter = center - osg::Vec2f(-0.25f,  0.25f); break;
            case 0: realCenter = center - osg::Vec2f(-0.25f, -0.25f); break;
            default: realCenter = center; break;
        }
        const osg::Vec2f origin2 = realCenter - osg::Vec2f(1.f, 1.f) * 0.5f; // assumed to be bottom left corner
        //std::cout << origin2.x() << ", " << origin2.y() << std::endl;
#endif
        const osg::Vec2f origin = center - osg::Vec2f(size, size) * 0.5f;
        //std::cout << origin.x() << ", " << origin.y() << std::endl;
        const int startCellX = static_cast<int>(std::floor(origin.x()));
        const int startCellY = static_cast<int>(std::floor(origin.y()));
        LandCache cache(startCellX - 1, startCellY - 1, static_cast<std::size_t>(std::ceil(size)) + 2);
        std::pair lastCell{ startCellX, startCellY };
        const LandObject* land = getLand(ESM::ExteriorCellLocation(startCellX, startCellY, worldspace), cache);
        const ESM::LandData* heightData = nullptr;
        const ESM::LandData* normalData = nullptr;
        const ESM::LandData* colourData = nullptr;
        bool validHeightDataExists = false;

        if (land != nullptr)
        {
            heightData = land->getData(ESM::Land::DATA_VHGT);
            normalData = land->getData(ESM::Land::DATA_VNML);
            colourData = land->getData(ESM::Land::DATA_VCLR);
            validHeightDataExists = true;
        }

        int rowStart = 0;
        int colStart = 0;
        int rowEnd, colEnd;

        // FIXME: how to ignore the repeat of left/bottom quad?
        switch (quad)
        {
            case 0: // bottom left
            {
                rowStart = 0;
                colStart = 0;

                rowEnd = int(cellSize / 2) + 1; // int(33 / 2) + 1 = 17
                colEnd = int(cellSize / 2) + 1;

                break;
            }
            case 2: // bottom right
            {
                rowStart = 0;
                colStart = int(cellSize / 2); // 16, repeat the last of the left quad

                rowEnd = int(cellSize / 2) + 1; // 17
                colEnd = cellSize;

                break;
            }
            case 1: // top left
            {
                rowStart = int(cellSize / 2); // 16, repeat the last of the bottom quad
                colStart = 0;

                rowEnd = cellSize;
                colEnd = int(cellSize / 2) + 1; // 17

                break;
            }
            case 3: // top right
            {
                rowStart = int(cellSize / 2); // 16
                colStart = int(cellSize / 2); // 16

                rowEnd = cellSize; // 33
                colEnd = cellSize; // 33

                break;
            }
            default:
                std::fill(positions.begin(), positions.end(), osg::Vec3f());
                return; // FIXME: throw instead?
        }

        osg::Vec3f normal(0, 0, 1);
        osg::Vec4ub color(255, 255, 255, 255);

        // ESM4::Land::mLandData.mHeights start at the bottom left hand corner
        //
        //               row
        //                |
        //                v
        // 1056 ..1088   32
        // 1023 ..1055   31
        //      ..
        //   99 .. 131    3
        //   66 ..  98    2
        //   33 ..  65    1
        //    0 ..  32    0
        //
        //    0 ..  32  <- col
        //
        // row and col represent cell space (i.e. mHeights, mVertNorm and mVertColr)
        // vertX and vertY represent quad space
        float vertY = 0;
        float vertX = 0;
        for (int col = colStart; col < colEnd; col += 1)
        {
            vertX = 0;
            for (int row = rowStart; row < rowEnd; row += 1)
            {
                float height = -2048;
                if (land && heightData) // validHeightDataExists
                    height = heightData->getHeights()[col*cellSize + row];

                // FIXME: I suspect landSizeInUnits should be 2048
                const std::size_t vertIndex = vertX * numVerts + vertY;
                positions[vertIndex]
                    = osg::Vec3f((vertX / static_cast<float>(numVerts - 1) - 0.5f) * size * landSizeInUnits,
                                 (vertY / static_cast<float>(numVerts - 1) - 0.5f) * size * landSizeInUnits,
                                 height);

                if (land && normalData)
                {
                    normal.x() = normalData->getNormals()[col * cellSize * 3 + row * 3 + 0];
                    normal.y() = normalData->getNormals()[col * cellSize * 3 + row * 3 + 1];
                    normal.z() = normalData->getNormals()[col * cellSize * 3 + row * 3 + 2];
                    normal.normalize();
                }
                else
                    normal = osg::Vec3f(0, 0, 1);

// FIXME: not sure if below normal fixes for Morrowind also applies to TES4
// TODO: needs testing
#if 0
                // Normals apparently don't connect seamlessly between cells
                if (col == cellSize - 1 || row == cellSize - 1)
                    fixNormal(normal, cellLocation, col, row, cache);

                // some corner normals appear to be complete garbage (z < 0)
                if ((row == 0 || row == cellSize - 1) && (col == 0 || col == cellSize - 1))
                    averageNormal(normal, cellLocation, col, row, cache);
#endif
                //assert(normal.z() > 0); // ToddLand triggers this
                if (normal.z() < 0)
                    normal.z() = 0;

                normals[vertIndex] = normal;

                if (land && colourData)
                {
                    color.r() = colourData->getColors()[col * cellSize * 3 + row * 3 + 0];
                    color.g() = colourData->getColors()[col * cellSize * 3 + row * 3 + 1];
                    color.b() = colourData->getColors()[col * cellSize * 3 + row * 3 + 2];
                }
                else
                {
                    color.r() = 1;
                    color.g() = 1;
                    color.b() = 1;
                }

// FIXME: not sure if below colour fixes for Morrowind also applies to TES4
// TODO: needs testing
#if 0
                // Unlike normals, colors mostly connect seamlessly between cells, but not always...
                if (col == cellSize - 1 || row == cellSize - 1)
                    fixColour(color, cellLocation, col, row, cache);
#endif
//              color.a() = 1;
                colours[vertIndex] = color;

                ++vertX;
            }
            ++vertY;
        }
    }

    void Storage::getQuadBlendmaps(float chunkSize, const osg::Vec2f& chunkCenter, ImageVector& blendmaps,
            std::vector<Terrain::LayerInfo>& layerList, ESM::RefId worldspace, int quad)
    {
        // VTXT info indicates texture size is 17x17 - but the cell grid is 33x33
        // (cf. TES3 has 65x65 cell) do we discard one row and column or overlap?
        //
        // NOTE: each base texture does not completely "fill" a quadrant.  The observations in
        // TES4 vanilla indicates that the texture repeats (or "wraps") 6 times each side
        //
        //     ///////////////// ////////////////   <-- discard texture row?
        //    +-----------------+----------------+/
        // 32 |\               \|                |/
        // 31 |\               \|                |/
        //    |\     17x16     \|      16x16     |/
        //  . |\               \|                |/
        //  . |\       2       \|        3       |/
        //  . |\               \|                |/
        //  . |\               \<---------------------- overlap column instead?
        // 17 |\               \|                |/
        //    +-----------------+----------------+
        // 16 |\                |\\\\\\\\\\\\\\\\|<---- overlap row instead?
        // 15 |\                |                |/
        //  . |\     17x17      |      16x17     |/
        //  . |\                |                |/
        //  . |\       0        |        1       |/
        //  . |\                |                |/
        //  2 |\                |                |/
        //  1 |\                |                |/
        //  0 |\\\\\\\\\\\\\\\\\|\\\\\\\\\\\\\\\\|<---- this row of vertices is a copy of cell below
        //    +-----------------+----------------+
        //                   111 1             33 ^
        //     0123  ......  456 7    .....    12 |
        //     ^                                 discard texture column?
        //     |
        //    this column of vertices is a copy of the cell to the left
        //
        const osg::Vec2f origin = chunkCenter - osg::Vec2f(chunkSize, chunkSize) * 0.5f;
        const int startCellX = static_cast<int>(std::floor(origin.x()));
        const int startCellY = static_cast<int>(std::floor(origin.y()));
        const int realTextureSize = 17; // FIXME: should be defined in Land record
        //const std::size_t blendmapSize = getBlendmapSize(chunkSize, realTextureSize);
        const std::size_t blendmapSize = realTextureSize;

        // FIXME: temp testing
        //if (startCellX == 12 && startCellY == 21)
            //std::cout << "vilverin exterior" << std::endl;

// FIXME: I don't think ESM4 needs this?
#if 0
        // We need to upscale the blendmap 2x with nearest neighbor sampling to look like Vanilla
        constexpr std::size_t imageScaleFactor = 2;
#else
        constexpr std::size_t imageScaleFactor = 1;
#endif
        const std::size_t blendmapImageSize = blendmapSize * imageScaleFactor;
        std::vector<UniqueTextureId> textureIds(blendmapSize * blendmapSize);

// NOTE: we need all the texture data which are missing in LandObject
#if 0
        LandCache cache(startCellX - 1, startCellY - 1, static_cast<std::size_t>(std::ceil(chunkSize)) + 2);
        std::pair lastCell{ startCellX, startCellY };
        const LandObject* land = getLand(ESM::ExteriorCellLocation(startCellX, startCellY, worldspace), cache);
#endif
        // FIXME: do we need to cache this data? (already in ESMStore, why cache again?)
        //        alternatively modify LandObject with all the extra data rather than use getEsm4Land()?
        const ESM4::Land* land = getEsm4Land(ESM::ExteriorCellLocation(startCellX, startCellY, worldspace));
        if (!land)
            return; // FIXME: throw instead?

// I don't think we need this?
#if 0
        const auto handleSample = [&](const CellSample& sample) {
            const std::pair cell{ sample.mCellX, sample.mCellY };
            if (lastCell != cell)
            {
                land = getEsm4Land(ESM::ExteriorCellLocation(sample.mCellX, sample.mCellY, worldspace));
                lastCell = cell;
            }

            textureIds[sample.mDstCol * blendmapSize + sample.mDstRow]
                = getQuadTextureIdAt(land, sample.mSrcRow, sample.mSrcCol);
        };

        sampleBlendmaps(chunkSize, origin.x(), origin.y(), realTextureSize, handleSample);

        std::map<UniqueTextureId, std::size_t> textureIndicesMap;
#endif
        // FIXME: debugging only
        //std::cout << "quad " << quad << std::endl;

        // base texture
        Terrain::LayerInfo info;
        ESM::FormId ltexId = ESM::FormId::fromUint32(land->mTextures[quad].base.formId);
        std::string texture = getEsm4TextureName(ltexId);
        if (texture == "")
            info = getLayerInfo(getEsm4TextureSet(ltexId)); // FO3/FONV/TES5
        else
            info = getLayerInfo(texture); // TES4

        // FIXME: debugging only
        //std::cout << "base " << info.mDiffuseMap << std::endl;
        osg::ref_ptr<osg::Image> image(new osg::Image);
        image->allocateImage(static_cast<int>(blendmapImageSize), static_cast<int>(blendmapImageSize),
            1, GL_ALPHA, GL_UNSIGNED_BYTE);
        std::memset(image->data(), 255, image->getTotalDataSize()); // fully opaque for base texture
        blendmaps.push_back(std::move(image));
        layerList.push_back(std::move(info));

        // additional textures

        std::size_t numLayers = land->mTextures[quad].layers.size();
        for (std::size_t i = 0; i < numLayers; ++i)
        {
            Terrain::LayerInfo layerInfo;
            /*ESM::FormId*/ ltexId = ESM::FormId::fromUint32(land->mTextures[quad].layers[i].texture.formId);
            std::string layerTexture = getEsm4TextureName(ltexId);
            if (layerTexture == "")
                layerInfo = getLayerInfo(getEsm4TextureSet(ltexId)); // FO3/FONV/TES5
            else
                layerInfo = getLayerInfo(layerTexture); // TES4

            // FIXME: debugging only
            //std::cout << "layer " << i << ", " << layerInfo.mDiffuseMap << std::endl;

            osg::ref_ptr<osg::Image> layerImage(new osg::Image);
            layerImage->allocateImage(static_cast<int>(blendmapImageSize), static_cast<int>(blendmapImageSize),
                1, GL_ALPHA, GL_UNSIGNED_BYTE);
            std::memset(layerImage->data(), 0, layerImage->getTotalDataSize());
            blendmaps.push_back(std::move(layerImage));
            layerList.push_back(std::move(layerInfo));

            const std::size_t layerIndex = blendmaps.size() - 1;
            unsigned char* const data = blendmaps[layerIndex]->data();

            // osg::Image default origin is bottom left and VTXT data also starts at bottom left
            // corner i.e. there should be no conversion required
            //
            // FIXME: but the observed behaviour is different - either VTXT starts at top left
            //        corner or osg::Image is being interpreted differently by the shader
            //
            // Image      guessed VTXT
            // index       position    y'
            //
            // 272 ..288     0 .. 16   0
            //     ..          ..
            //  51 .. 67   221 ..237  13
            //  34 .. 50   238 ..254  14
            //  17 .. 33   255 ..271  15
            //   0 .. 16   272 ..288  16
            //
            // y  = floor(position / 17)
            // y' = 17 - 1 - y
            // x  = position % 17
            //
            // e.g. position = 275, y = 16, y' = 0,  x = 3
            //      position = 50,  y = 2,  y' = 14, x = 16
            const std::vector<ESM4::Land::VTXT>& opacityData = land->mTextures[quad].layers[i].data;
            for (std::size_t j = 0; j < opacityData.size(); ++j)
            {
                // NOTE: blendmapImageSize, blendmapSize and realTextureSize are all the same (17)

                int position = opacityData[j].position;

                std::size_t y = realTextureSize - 1 - std::floor(position / realTextureSize);
                std::size_t x = position % realTextureSize;
                data[y*realTextureSize + x] = unsigned char(opacityData[j].opacity * 255);
            }
        }
    }

}
