#include "tilecachedrecastmeshmanager.hpp"
#include "makenavmesh.hpp"
#include "gettilespositions.hpp"
#include "settingsutils.hpp"
#include "changetype.hpp"

#include <components/debug/debuglog.hpp>
#include <components/misc/convert.hpp>

#include <algorithm>
#include <vector>
#include <limits>

namespace DetourNavigator
{
    namespace
    {
        const TileBounds infiniteTileBounds {
            osg::Vec2f(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()),
            osg::Vec2f(std::numeric_limits<float>::max(), std::numeric_limits<float>::max())
        };
    }

    TileCachedRecastMeshManager::TileCachedRecastMeshManager(const RecastSettings& settings)
        : mSettings(settings)
        , mBounds(infiniteTileBounds)
        , mRange(makeTilesPositionsRange(mBounds.mMin, mBounds.mMax, mSettings))
    {}

    TileBounds TileCachedRecastMeshManager::getBounds() const
    {
        return mBounds;
    }

    std::vector<std::pair<TilePosition, ChangeType>> TileCachedRecastMeshManager::setBounds(const TileBounds& bounds)
    {
        std::vector<std::pair<TilePosition, ChangeType>> changedTiles;

        if (mBounds == bounds)
            return changedTiles;

        const auto newRange = makeTilesPositionsRange(bounds.mMin, bounds.mMax, mSettings);

        if (mBounds != infiniteTileBounds)
        {
            const auto locked = mWorldspaceTiles.lock();
            for (auto& object : mObjects)
            {
                const ObjectId id = object.first;
                ObjectData& data = object.second;
                const TilesPositionsRange objectRange = makeTilesPositionsRange(data.mShape.getShape(), data.mTransform, mSettings);

                const auto onOldTilePosition = [&] (const TilePosition& position)
                {
                    if (isInTilesPositionsRange(newRange, position))
                        return;
                    const auto it = data.mTiles.find(position);
                    if (it == data.mTiles.end())
                        return;
                    data.mTiles.erase(it);
                    if (removeTile(id, position, locked->mTiles))
                        changedTiles.emplace_back(position, ChangeType::remove);
                };
                getTilesPositions(getIntersection(mRange, objectRange), onOldTilePosition);

                const auto onNewTilePosition = [&] (const TilePosition& position)
                {
                    if (data.mTiles.find(position) != data.mTiles.end())
                        return;
                    if (addTile(id, data.mShape, data.mTransform, data.mAreaType, position, locked->mTiles))
                    {
                        data.mTiles.insert(position);
                        changedTiles.emplace_back(position, ChangeType::add);
                    }
                };
                getTilesPositions(getIntersection(newRange, objectRange), onNewTilePosition);
            }

            std::sort(changedTiles.begin(), changedTiles.end());
            changedTiles.erase(std::unique(changedTiles.begin(), changedTiles.end()), changedTiles.end());
        }

        if (!changedTiles.empty())
            ++mRevision;

        mBounds = bounds;
        mRange = newRange;

        return changedTiles;
    }

    std::string TileCachedRecastMeshManager::getWorldspace() const
    {
        return mWorldspaceTiles.lockConst()->mWorldspace;
    }

    void TileCachedRecastMeshManager::setWorldspace(std::string_view worldspace)
    {
        const auto locked = mWorldspaceTiles.lock();
        if (locked->mWorldspace == worldspace)
            return;
        locked->mTiles.clear();
        locked->mWorldspace = worldspace;
    }

    std::optional<RemovedRecastMeshObject> TileCachedRecastMeshManager::removeObject(const ObjectId id)
    {
        const auto object = mObjects.find(id);
        if (object == mObjects.end())
            return std::nullopt;
        std::optional<RemovedRecastMeshObject> result;
        {
            const auto locked = mWorldspaceTiles.lock();
            for (const auto& tilePosition : object->second.mTiles)
            {
                const auto removed = removeTile(id, tilePosition, locked->mTiles);
                if (removed && !result)
                    result = removed;
            }
        }
        mObjects.erase(object);
        if (result)
            ++mRevision;
        return result;
    }

    bool TileCachedRecastMeshManager::addWater(const osg::Vec2i& cellPosition, int cellSize, float level)
    {
        const auto it = mWaterTilesPositions.find(cellPosition);
        if (it != mWaterTilesPositions.end())
            return false;

        std::vector<TilePosition>& tilesPositions = mWaterTilesPositions.emplace_hint(
            it, cellPosition, std::vector<TilePosition>())->second;

        bool result = false;

        if (cellSize == std::numeric_limits<int>::max())
        {
            const auto locked = mWorldspaceTiles.lock();
            for (auto& tile : locked->mTiles)
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
            getTilesPositions(makeTilesPositionsRange(cellSize, shift, mSettings),
                [&] (const TilePosition& tilePosition)
                {
                    const auto locked = mWorldspaceTiles.lock();
                    auto tile = locked->mTiles.find(tilePosition);
                    if (tile == locked->mTiles.end())
                    {
                        const TileBounds tileBounds = makeRealTileBoundsWithBorder(mSettings, tilePosition);
                        tile = locked->mTiles.emplace_hint(tile, tilePosition,
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
            const auto locked = mWorldspaceTiles.lock();
            const auto tile = locked->mTiles.find(tilePosition);
            if (tile == locked->mTiles.end())
                continue;
            const auto tileResult = tile->second->removeWater(cellPosition);
            if (tile->second->isEmpty())
            {
                locked->mTiles.erase(tile);
                ++mTilesGeneration;
            }
            if (tileResult && !result)
                result = tileResult;
        }
        mWaterTilesPositions.erase(object);
        if (result)
            ++mRevision;
        return result;
    }

    bool TileCachedRecastMeshManager::addHeightfield(const osg::Vec2i& cellPosition, int cellSize,
        const HeightfieldShape& shape)
    {
        const auto it = mHeightfieldTilesPositions.find(cellPosition);
        if (it != mHeightfieldTilesPositions.end())
            return false;

        std::vector<TilePosition>& tilesPositions = mHeightfieldTilesPositions.emplace_hint(
            it, cellPosition, std::vector<TilePosition>())->second;
        const btVector3 shift = getHeightfieldShift(shape, cellPosition, cellSize);

        bool result = false;

        getTilesPositions(makeTilesPositionsRange(cellSize, shift, mSettings),
            [&] (const TilePosition& tilePosition)
            {
                const auto locked = mWorldspaceTiles.lock();
                auto tile = locked->mTiles.find(tilePosition);
                if (tile == locked->mTiles.end())
                {
                    const TileBounds tileBounds = makeRealTileBoundsWithBorder(mSettings, tilePosition);
                    tile = locked->mTiles.emplace_hint(tile, tilePosition,
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
            const auto locked = mWorldspaceTiles.lock();
            const auto tile = locked->mTiles.find(tilePosition);
            if (tile == locked->mTiles.end())
                continue;
            const auto tileResult = tile->second->removeHeightfield(cellPosition);
            if (tile->second->isEmpty())
            {
                locked->mTiles.erase(tile);
                ++mTilesGeneration;
            }
            if (tileResult && !result)
                result = tileResult;
        }
        mHeightfieldTilesPositions.erase(object);
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
        const auto locked = mWorldspaceTiles.lockConst();
        const auto it = locked->mTiles.find(tilePosition);
        if (it == locked->mTiles.end())
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
        const auto locked = mWorldspaceTiles.lockConst();
        if (locked->mWorldspace != worldspace)
            return nullptr;
        const auto it = locked->mTiles.find(tilePosition);
        if (it == locked->mTiles.end())
            return nullptr;
        return it->second;
    }
}
