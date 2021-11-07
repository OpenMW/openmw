#include "tilecachedrecastmeshmanager.hpp"
#include "makenavmesh.hpp"
#include "gettilespositions.hpp"
#include "settingsutils.hpp"

#include <components/debug/debuglog.hpp>

#include <algorithm>
#include <vector>

namespace DetourNavigator
{
    TileCachedRecastMeshManager::TileCachedRecastMeshManager(const Settings& settings)
        : mSettings(settings)
    {}

    bool TileCachedRecastMeshManager::addObject(const ObjectId id, const CollisionShape& shape,
                                                const btTransform& transform, const AreaType areaType)
    {
        std::vector<TilePosition> tilesPositions;
        {
            auto tiles = mTiles.lock();
            getTilesPositions(shape.getShape(), transform, mSettings, [&] (const TilePosition& tilePosition)
                {
                    if (addTile(id, shape, transform, areaType, tilePosition, tiles.get()))
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
            auto tiles = mTiles.lock();
            for (const auto& tilePosition : object->second)
            {
                const auto removed = removeTile(id, tilePosition, tiles.get());
                if (removed && !result)
                    result = removed;
            }
        }
        if (result)
            ++mRevision;
        return result;
    }

    bool TileCachedRecastMeshManager::addWater(const osg::Vec2i& cellPosition, const int cellSize,
        const osg::Vec3f& shift)
    {
        auto& tilesPositions = mWaterTilesPositions[cellPosition];

        bool result = false;

        if (cellSize == std::numeric_limits<int>::max())
        {
            const auto tiles = mTiles.lock();
            for (auto& tile : *tiles)
            {
                if (tile.second->addWater(cellPosition, cellSize, shift))
                {
                    tilesPositions.push_back(tile.first);
                    result = true;
                }
            }
        }
        else
        {
            getTilesPositions(cellSize, shift, mSettings, [&] (const TilePosition& tilePosition)
                {
                    const auto tiles = mTiles.lock();
                    auto tile = tiles->find(tilePosition);
                    if (tile == tiles->end())
                    {
                        const TileBounds tileBounds = makeRealTileBoundsWithBorder(mSettings, tilePosition);
                        tile = tiles->emplace(tilePosition,
                                std::make_shared<CachedRecastMeshManager>(tileBounds, mTilesGeneration)).first;
                    }
                    if (tile->second->addWater(cellPosition, cellSize, shift))
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

    std::optional<Cell> TileCachedRecastMeshManager::removeWater(const osg::Vec2i& cellPosition)
    {
        const auto object = mWaterTilesPositions.find(cellPosition);
        if (object == mWaterTilesPositions.end())
            return std::nullopt;
        std::optional<Cell> result;
        for (const auto& tilePosition : object->second)
        {
            const auto tiles = mTiles.lock();
            const auto tile = tiles->find(tilePosition);
            if (tile == tiles->end())
                continue;
            const auto tileResult = tile->second->removeWater(cellPosition);
            if (tile->second->isEmpty())
            {
                tiles->erase(tile);
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
        const osg::Vec3f& shift, const HeightfieldShape& shape)
    {
        auto& tilesPositions = mHeightfieldTilesPositions[cellPosition];

        bool result = false;

        getTilesPositions(cellSize, shift, mSettings, [&] (const TilePosition& tilePosition)
            {
                const auto tiles = mTiles.lock();
                auto tile = tiles->find(tilePosition);
                if (tile == tiles->end())
                {
                    const TileBounds tileBounds = makeRealTileBoundsWithBorder(mSettings, tilePosition);
                    tile = tiles->emplace(tilePosition,
                            std::make_shared<CachedRecastMeshManager>(tileBounds, mTilesGeneration)).first;
                }
                if (tile->second->addHeightfield(cellPosition, cellSize, shift, shape))
                {
                    tilesPositions.push_back(tilePosition);
                    result = true;
                }
            });

        if (result)
            ++mRevision;

        return result;
    }

    std::optional<Cell> TileCachedRecastMeshManager::removeHeightfield(const osg::Vec2i& cellPosition)
    {
        const auto object = mHeightfieldTilesPositions.find(cellPosition);
        if (object == mHeightfieldTilesPositions.end())
            return std::nullopt;
        std::optional<Cell> result;
        for (const auto& tilePosition : object->second)
        {
            const auto tiles = mTiles.lock();
            const auto tile = tiles->find(tilePosition);
            if (tile == tiles->end())
                continue;
            const auto tileResult = tile->second->removeHeightfield(cellPosition);
            if (tile->second->isEmpty())
            {
                tiles->erase(tile);
                ++mTilesGeneration;
            }
            if (tileResult && !result)
                result = tileResult;
        }
        if (result)
            ++mRevision;
        return result;
    }

    std::shared_ptr<RecastMesh> TileCachedRecastMeshManager::getMesh(const TilePosition& tilePosition) const
    {
        if (const auto manager = getManager(tilePosition))
            return manager->getMesh();
        return nullptr;
    }

    std::shared_ptr<RecastMesh> TileCachedRecastMeshManager::getCachedMesh(const TilePosition& tilePosition) const
    {
        if (const auto manager = getManager(tilePosition))
            return manager->getCachedMesh();
        return nullptr;
    }

    std::size_t TileCachedRecastMeshManager::getRevision() const
    {
        return mRevision;
    }

    void TileCachedRecastMeshManager::reportNavMeshChange(const TilePosition& tilePosition, Version recastMeshVersion, Version navMeshVersion) const
    {
        const auto tiles = mTiles.lockConst();
        const auto it = tiles->find(tilePosition);
        if (it == tiles->end())
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
            tile = tiles.emplace(tilePosition,
                    std::make_shared<CachedRecastMeshManager>(tileBounds, mTilesGeneration)).first;
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

    std::shared_ptr<CachedRecastMeshManager> TileCachedRecastMeshManager::getManager(const TilePosition& tilePosition) const
    {
        const auto tiles = mTiles.lockConst();
        const auto it = tiles->find(tilePosition);
        if (it == tiles->end())
            return nullptr;
        return it->second;
    }
}
