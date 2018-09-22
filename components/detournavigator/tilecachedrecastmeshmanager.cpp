#include "tilecachedrecastmeshmanager.hpp"
#include "makenavmesh.hpp"
#include "gettilespositions.hpp"
#include "settingsutils.hpp"

namespace DetourNavigator
{
    TileCachedRecastMeshManager::TileCachedRecastMeshManager(const Settings& settings)
        : mSettings(settings)
    {}

    bool TileCachedRecastMeshManager::addObject(const ObjectId id, const btCollisionShape& shape,
                                                const btTransform& transform, const AreaType areaType)
    {
        bool result = false;
        auto& tilesPositions = mObjectsTilesPositions[id];
        const auto border = getBorderSize(mSettings);
        getTilesPositions(shape, transform, mSettings, [&] (const TilePosition& tilePosition)
            {
                std::unique_lock<std::mutex> lock(mTilesMutex);
                auto tile = mTiles.find(tilePosition);
                if (tile == mTiles.end())
                {
                    auto tileBounds = makeTileBounds(mSettings, tilePosition);
                    tileBounds.mMin -= osg::Vec2f(border, border);
                    tileBounds.mMax += osg::Vec2f(border, border);
                    tile = mTiles.insert(std::make_pair(tilePosition,
                            CachedRecastMeshManager(mSettings, tileBounds))).first;
                }
                if (tile->second.addObject(id, shape, transform, areaType))
                {
                    lock.unlock();
                    tilesPositions.push_back(tilePosition);
                    result = true;
                }
            });
        if (result)
            ++mRevision;
        return result;
    }

    bool TileCachedRecastMeshManager::updateObject(const ObjectId id, const btTransform& transform,
        const AreaType areaType)
    {
        const auto object = mObjectsTilesPositions.find(id);
        if (object == mObjectsTilesPositions.end())
            return false;
        bool result = false;
        std::unique_lock<std::mutex> lock(mTilesMutex);
        for (const auto& tilePosition : object->second)
        {
            const auto tile = mTiles.find(tilePosition);
            if (tile != mTiles.end())
                result = tile->second.updateObject(id, transform, areaType) || result;
        }
        lock.unlock();
        if (result)
            ++mRevision;
        return result;
    }

    boost::optional<RemovedRecastMeshObject> TileCachedRecastMeshManager::removeObject(const ObjectId id)
    {
        const auto object = mObjectsTilesPositions.find(id);
        if (object == mObjectsTilesPositions.end())
            return boost::none;
        boost::optional<RemovedRecastMeshObject> result;
        for (const auto& tilePosition : object->second)
        {
            std::unique_lock<std::mutex> lock(mTilesMutex);
            const auto tile = mTiles.find(tilePosition);
            if (tile == mTiles.end())
                continue;
            const auto tileResult = tile->second.removeObject(id);
            if (tile->second.isEmpty())
                mTiles.erase(tile);
            lock.unlock();
            if (tileResult && !result)
                result = tileResult;
        }
        if (result)
            ++mRevision;
        return result;
    }

    bool TileCachedRecastMeshManager::addWater(const osg::Vec2i& cellPosition, const int cellSize,
        const btTransform& transform)
    {
        const auto border = getBorderSize(mSettings);

        auto& tilesPositions = mWaterTilesPositions[cellPosition];

        bool result = false;

        if (cellSize == std::numeric_limits<int>::max())
        {
            const std::lock_guard<std::mutex> lock(mTilesMutex);
            for (auto& tile : mTiles)
            {
                if (tile.second.addWater(cellPosition, cellSize, transform))
                {
                    tilesPositions.push_back(tile.first);
                    result = true;
                }
            }
        }
        else
        {
            getTilesPositions(cellSize, transform, mSettings, [&] (const TilePosition& tilePosition)
                {
                    std::unique_lock<std::mutex> lock(mTilesMutex);
                    auto tile = mTiles.find(tilePosition);
                    if (tile == mTiles.end())
                    {
                        auto tileBounds = makeTileBounds(mSettings, tilePosition);
                        tileBounds.mMin -= osg::Vec2f(border, border);
                        tileBounds.mMax += osg::Vec2f(border, border);
                        tile = mTiles.insert(std::make_pair(tilePosition,
                                CachedRecastMeshManager(mSettings, tileBounds))).first;
                    }
                    if (tile->second.addWater(cellPosition, cellSize, transform))
                    {
                        lock.unlock();
                        tilesPositions.push_back(tilePosition);
                        result = true;
                    }
                });
        }

        if (result)
            ++mRevision;

        return result;
    }

    boost::optional<RecastMeshManager::Water> TileCachedRecastMeshManager::removeWater(const osg::Vec2i& cellPosition)
    {
        const auto object = mWaterTilesPositions.find(cellPosition);
        if (object == mWaterTilesPositions.end())
            return boost::none;
        boost::optional<RecastMeshManager::Water> result;
        for (const auto& tilePosition : object->second)
        {
            std::unique_lock<std::mutex> lock(mTilesMutex);
            const auto tile = mTiles.find(tilePosition);
            if (tile == mTiles.end())
                continue;
            const auto tileResult = tile->second.removeWater(cellPosition);
            if (tile->second.isEmpty())
                mTiles.erase(tile);
            lock.unlock();
            if (tileResult && !result)
                result = tileResult;
        }
        if (result)
            ++mRevision;
        return result;
    }

    std::shared_ptr<RecastMesh> TileCachedRecastMeshManager::getMesh(const TilePosition& tilePosition)
    {
        const std::lock_guard<std::mutex> lock(mTilesMutex);
        const auto it = mTiles.find(tilePosition);
        if (it == mTiles.end())
            return nullptr;
        return it->second.getMesh();
    }

    bool TileCachedRecastMeshManager::hasTile(const TilePosition& tilePosition)
    {
        const std::lock_guard<std::mutex> lock(mTilesMutex);
        return mTiles.count(tilePosition);
    }

    std::size_t TileCachedRecastMeshManager::getRevision() const
    {
        return mRevision;
    }
}
