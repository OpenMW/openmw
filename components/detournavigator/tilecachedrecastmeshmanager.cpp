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
                const auto tiles = mTiles.lock();
                auto tile = tiles->find(tilePosition);
                if (tile == tiles->end())
                {
                    auto tileBounds = makeTileBounds(mSettings, tilePosition);
                    tileBounds.mMin -= osg::Vec2f(border, border);
                    tileBounds.mMax += osg::Vec2f(border, border);
                    tile = tiles->insert(std::make_pair(tilePosition,
                            CachedRecastMeshManager(mSettings, tileBounds))).first;
                }
                if (tile->second.addObject(id, shape, transform, areaType))
                {
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
        {
            const auto tiles = mTiles.lock();
            for (const auto& tilePosition : object->second)
            {
                const auto tile = tiles->find(tilePosition);
                if (tile != tiles->end())
                    result = tile->second.updateObject(id, transform, areaType) || result;
            }
        }
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
            const auto tiles = mTiles.lock();
            const auto tile = tiles->find(tilePosition);
            if (tile == tiles->end())
                continue;
            const auto tileResult = tile->second.removeObject(id);
            if (tile->second.isEmpty())
                tiles->erase(tile);
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
            const auto tiles = mTiles.lock();
            for (auto& tile : *tiles)
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
                    const auto tiles = mTiles.lock();
                    auto tile = tiles->find(tilePosition);
                    if (tile == tiles->end())
                    {
                        auto tileBounds = makeTileBounds(mSettings, tilePosition);
                        tileBounds.mMin -= osg::Vec2f(border, border);
                        tileBounds.mMax += osg::Vec2f(border, border);
                        tile = tiles->insert(std::make_pair(tilePosition,
                                CachedRecastMeshManager(mSettings, tileBounds))).first;
                    }
                    if (tile->second.addWater(cellPosition, cellSize, transform))
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

    boost::optional<RecastMeshManager::Water> TileCachedRecastMeshManager::removeWater(const osg::Vec2i& cellPosition)
    {
        const auto object = mWaterTilesPositions.find(cellPosition);
        if (object == mWaterTilesPositions.end())
            return boost::none;
        boost::optional<RecastMeshManager::Water> result;
        for (const auto& tilePosition : object->second)
        {
            const auto tiles = mTiles.lock();
            const auto tile = tiles->find(tilePosition);
            if (tile == tiles->end())
                continue;
            const auto tileResult = tile->second.removeWater(cellPosition);
            if (tile->second.isEmpty())
                tiles->erase(tile);
            if (tileResult && !result)
                result = tileResult;
        }
        if (result)
            ++mRevision;
        return result;
    }

    std::shared_ptr<RecastMesh> TileCachedRecastMeshManager::getMesh(const TilePosition& tilePosition)
    {
        const auto tiles = mTiles.lock();
        const auto it = tiles->find(tilePosition);
        if (it == tiles->end())
            return nullptr;
        return it->second.getMesh();
    }

    bool TileCachedRecastMeshManager::hasTile(const TilePosition& tilePosition)
    {
        return mTiles.lockConst()->count(tilePosition);
    }

    std::size_t TileCachedRecastMeshManager::getRevision() const
    {
        return mRevision;
    }
}
