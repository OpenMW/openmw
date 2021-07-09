#include "tilecachedrecastmeshmanager.hpp"
#include "makenavmesh.hpp"
#include "gettilespositions.hpp"
#include "settingsutils.hpp"

#include <components/debug/debuglog.hpp>

#include <algorithm>
#include <vector>

namespace DetourNavigator
{
    TileCachedRecastMeshManager::TileCachedRecastMeshManager(const RecastSettings& settings)
        : mSettings(settings)
    {}

    std::string TileCachedRecastMeshManager::getWorldspace() const
    {
        const std::lock_guard lock(mMutex);
        return mWorldspace;
    }

    void TileCachedRecastMeshManager::setWorldspace(std::string_view worldspace)
    {
        const std::lock_guard lock(mMutex);
        if (mWorldspace == worldspace)
            return;
        mTiles.clear();
        mWorldspace = worldspace;
    }

    bool TileCachedRecastMeshManager::addObject(const ObjectId id, const CollisionShape& shape,
                                                const btTransform& transform, const AreaType areaType)
    {
        std::vector<TilePosition> tilesPositions;
        {
            const std::lock_guard lock(mMutex);
            getTilesPositions(shape.getShape(), transform, mSettings, [&] (const TilePosition& tilePosition)
                {
                    if (addTile(id, shape, transform, areaType, tilePosition, mTiles))
                        tilesPositions.push_back(tilePosition);
                });
        }
        if (tilesPositions.empty())
            return false;
        std::sort(tilesPositions.begin(), tilesPositions.end());
        mObjectsTilesPositions.insert_or_assign(id, std::move(tilesPositions));
        ++mRevision;
        return true;
    }

    std::optional<RemovedRecastMeshObject> TileCachedRecastMeshManager::removeObject(const ObjectId id)
    {
        const auto object = mObjectsTilesPositions.find(id);
        if (object == mObjectsTilesPositions.end())
            return std::nullopt;
        std::optional<RemovedRecastMeshObject> result;
        {
            const std::lock_guard lock(mMutex);
            for (const auto& tilePosition : object->second)
            {
                const auto removed = removeTile(id, tilePosition, mTiles);
                if (removed && !result)
                    result = removed;
            }
        }
        if (result)
            ++mRevision;
        return result;
    }

    bool TileCachedRecastMeshManager::addWater(const osg::Vec2i& cellPosition, int cellSize, float level)
    {
        auto& tilesPositions = mWaterTilesPositions[cellPosition];

        bool result = false;

        if (cellSize == std::numeric_limits<int>::max())
        {
            const std::lock_guard lock(mMutex);
            for (auto& tile : mTiles)
            {
                if (tile.second->addWater(cellPosition, cellSize, level))
                {
                    tilesPositions.push_back(tile.first);
                    result = true;
                }
            }
        }
        else
        {
            const btVector3 shift = Misc::Convert::toBullet(getWaterShift3d(cellPosition, cellSize, level));
            getTilesPositions(cellSize, shift, mSettings, [&] (const TilePosition& tilePosition)
                {
                    const std::lock_guard lock(mMutex);
                    auto tile = mTiles.find(tilePosition);
                    if (tile == mTiles.end())
                    {
                        const TileBounds tileBounds = makeRealTileBoundsWithBorder(mSettings, tilePosition);
                        tile = mTiles.emplace_hint(tile, tilePosition,
                                std::make_shared<CachedRecastMeshManager>(tileBounds, mTilesGeneration));
                    }
                    if (tile->second->addWater(cellPosition, cellSize, level))
                    {
                        tilesPositions.push_back(tilePosition);
                        result = true;
                    }
                });
        }

        if (result)
            ++mRevision;

        return result;
    }

    std::optional<Water> TileCachedRecastMeshManager::removeWater(const osg::Vec2i& cellPosition)
    {
        const auto object = mWaterTilesPositions.find(cellPosition);
        if (object == mWaterTilesPositions.end())
            return std::nullopt;
        std::optional<Water> result;
        for (const auto& tilePosition : object->second)
        {
            const std::lock_guard lock(mMutex);
            const auto tile = mTiles.find(tilePosition);
            if (tile == mTiles.end())
                continue;
            const auto tileResult = tile->second->removeWater(cellPosition);
            if (tile->second->isEmpty())
            {
                mTiles.erase(tile);
                ++mTilesGeneration;
            }
            if (tileResult && !result)
                result = tileResult;
        }
        if (result)
            ++mRevision;
        return result;
    }

    bool TileCachedRecastMeshManager::addHeightfield(const osg::Vec2i& cellPosition, int cellSize,
        const HeightfieldShape& shape)
    {
        const btVector3 shift = getHeightfieldShift(shape, cellPosition, cellSize);
        auto& tilesPositions = mHeightfieldTilesPositions[cellPosition];

        bool result = false;

        getTilesPositions(cellSize, shift, mSettings, [&] (const TilePosition& tilePosition)
            {
                const std::lock_guard lock(mMutex);
                auto tile = mTiles.find(tilePosition);
                if (tile == mTiles.end())
                {
                    const TileBounds tileBounds = makeRealTileBoundsWithBorder(mSettings, tilePosition);
                    tile = mTiles.emplace_hint(tile, tilePosition,
                            std::make_shared<CachedRecastMeshManager>(tileBounds, mTilesGeneration));
                }
                if (tile->second->addHeightfield(cellPosition, cellSize, shape))
                {
                    tilesPositions.push_back(tilePosition);
                    result = true;
                }
            });

        if (result)
            ++mRevision;

        return result;
    }

    std::optional<SizedHeightfieldShape> TileCachedRecastMeshManager::removeHeightfield(const osg::Vec2i& cellPosition)
    {
        const auto object = mHeightfieldTilesPositions.find(cellPosition);
        if (object == mHeightfieldTilesPositions.end())
            return std::nullopt;
        std::optional<SizedHeightfieldShape> result;
        for (const auto& tilePosition : object->second)
        {
            const std::lock_guard lock(mMutex);
            const auto tile = mTiles.find(tilePosition);
            if (tile == mTiles.end())
                continue;
            const auto tileResult = tile->second->removeHeightfield(cellPosition);
            if (tile->second->isEmpty())
            {
                mTiles.erase(tile);
                ++mTilesGeneration;
            }
            if (tileResult && !result)
                result = tileResult;
        }
        if (result)
            ++mRevision;
        return result;
    }

    std::shared_ptr<RecastMesh> TileCachedRecastMeshManager::getMesh(std::string_view worldspace, const TilePosition& tilePosition) const
    {
        if (const auto manager = getManager(worldspace, tilePosition))
            return manager->getMesh();
        return nullptr;
    }

    std::shared_ptr<RecastMesh> TileCachedRecastMeshManager::getCachedMesh(std::string_view worldspace, const TilePosition& tilePosition) const
    {
        if (const auto manager = getManager(worldspace, tilePosition))
            return manager->getCachedMesh();
        return nullptr;
    }

    std::shared_ptr<RecastMesh> TileCachedRecastMeshManager::getNewMesh(std::string_view worldspace, const TilePosition& tilePosition) const
    {
        if (const auto manager = getManager(worldspace, tilePosition))
            return manager->getNewMesh();
        return nullptr;
    }

    std::size_t TileCachedRecastMeshManager::getRevision() const
    {
        return mRevision;
    }

    void TileCachedRecastMeshManager::reportNavMeshChange(const TilePosition& tilePosition, Version recastMeshVersion, Version navMeshVersion) const
    {
        const std::lock_guard lock(mMutex);
        const auto it = mTiles.find(tilePosition);
        if (it == mTiles.end())
            return;
        it->second->reportNavMeshChange(recastMeshVersion, navMeshVersion);
    }

    bool TileCachedRecastMeshManager::addTile(const ObjectId id, const CollisionShape& shape,
        const btTransform& transform, const AreaType areaType, const TilePosition& tilePosition,
        TilesMap& tiles)
    {
        auto tile = tiles.find(tilePosition);
        if (tile == tiles.end())
        {
            const TileBounds tileBounds = makeRealTileBoundsWithBorder(mSettings, tilePosition);
            tile = tiles.emplace_hint(tile, tilePosition,
                    std::make_shared<CachedRecastMeshManager>(tileBounds, mTilesGeneration));
        }
        return tile->second->addObject(id, shape, transform, areaType);
    }

    bool TileCachedRecastMeshManager::updateTile(const ObjectId id, const btTransform& transform,
        const AreaType areaType, const TilePosition& tilePosition, TilesMap& tiles)
    {
        const auto tile = tiles.find(tilePosition);
        return tile != tiles.end() && tile->second->updateObject(id, transform, areaType);
    }

    std::optional<RemovedRecastMeshObject> TileCachedRecastMeshManager::removeTile(const ObjectId id,
        const TilePosition& tilePosition, TilesMap& tiles)
    {
        const auto tile = tiles.find(tilePosition);
        if (tile == tiles.end())
            return std::optional<RemovedRecastMeshObject>();
        auto tileResult = tile->second->removeObject(id);
        if (tile->second->isEmpty())
        {
            tiles.erase(tile);
            ++mTilesGeneration;
        }
        return tileResult;
    }

    std::shared_ptr<CachedRecastMeshManager> TileCachedRecastMeshManager::getManager(std::string_view worldspace,
        const TilePosition& tilePosition) const
    {
        const std::lock_guard lock(mMutex);
        if (mWorldspace != worldspace)
            return nullptr;
        const auto it = mTiles.find(tilePosition);
        if (it == mTiles.end())
            return nullptr;
        return it->second;
    }
}
