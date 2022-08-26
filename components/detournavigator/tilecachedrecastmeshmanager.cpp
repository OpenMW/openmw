#include "tilecachedrecastmeshmanager.hpp"
#include "gettilespositions.hpp"
#include "settingsutils.hpp"
#include "changetype.hpp"
#include "cachedrecastmeshmanager.hpp"

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

        bool updateTile(const ObjectId id, const btTransform& transform, const AreaType areaType,
            const TilePosition& tilePosition, std::map<TilePosition, std::shared_ptr<CachedRecastMeshManager>>& tiles)
        {
            const auto tile = tiles.find(tilePosition);
            return tile != tiles.end() && tile->second->updateObject(id, transform, areaType);
        }
    }

    TileCachedRecastMeshManager::TileCachedRecastMeshManager(const RecastSettings& settings)
        : mSettings(settings)
        , mBounds(infiniteTileBounds)
        , mRange(makeTilesPositionsRange(mBounds.mMin, mBounds.mMax, mSettings))
    {}

    void TileCachedRecastMeshManager::setBounds(const TileBounds& bounds)
    {
        if (mBounds == bounds)
            return;

        bool changed = false;
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
                    {
                        addChangedTile(position, ChangeType::remove);
                        changed = true;
                    }
                };
                getTilesPositions(getIntersection(mRange, objectRange), onOldTilePosition);

                const auto onNewTilePosition = [&] (const TilePosition& position)
                {
                    if (data.mTiles.find(position) != data.mTiles.end())
                        return;
                    if (addTile(id, data.mShape, data.mTransform, data.mAreaType, position, locked->mTiles))
                    {
                        data.mTiles.insert(position);
                        addChangedTile(position, ChangeType::add);
                    }
                };
                getTilesPositions(getIntersection(newRange, objectRange), onNewTilePosition);
            }
        }

        if (changed)
            ++mRevision;

        mBounds = bounds;
        mRange = newRange;
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

    bool TileCachedRecastMeshManager::addObject(const ObjectId id, const CollisionShape& shape,
        const btTransform& transform, const AreaType areaType)
    {
        auto it = mObjects.find(id);
        if (it != mObjects.end())
            return false;
        const TilesPositionsRange objectRange = makeTilesPositionsRange(shape.getShape(), transform, mSettings);
        const TilesPositionsRange range = getIntersection(mRange, objectRange);
        std::set<TilePosition> tilesPositions;
        if (range.mBegin != range.mEnd)
        {
            const auto locked = mWorldspaceTiles.lock();
            getTilesPositions(range,
                [&] (const TilePosition& tilePosition)
                {
                    if (addTile(id, shape, transform, areaType, tilePosition, locked->mTiles))
                    {
                        tilesPositions.insert(tilePosition);
                        addChangedTile(tilePosition, ChangeType::add);
                    }
                });
        }
        mObjects.emplace_hint(it, id, ObjectData {shape, transform, areaType, std::move(tilesPositions)});
        ++mRevision;
        return true;
    }

    bool TileCachedRecastMeshManager::updateObject(const ObjectId id, const CollisionShape& shape,
        const btTransform& transform, AreaType areaType)
    {
        const auto object = mObjects.find(id);
        if (object == mObjects.end())
            return false;
        auto& data = object->second;
        bool changed = false;
        std::set<TilePosition> newTiles;
        {
            const TilesPositionsRange objectRange = makeTilesPositionsRange(shape.getShape(), transform, mSettings);
            const TilesPositionsRange range = getIntersection(mRange, objectRange);
            const auto locked = mWorldspaceTiles.lock();
            const auto onTilePosition = [&] (const TilePosition& tilePosition)
            {
                if (data.mTiles.find(tilePosition) != data.mTiles.end())
                {
                    newTiles.insert(tilePosition);
                    if (updateTile(id, transform, areaType, tilePosition, locked->mTiles))
                    {
                        addChangedTile(tilePosition, ChangeType::update);
                        changed = true;
                    }
                }
                else if (addTile(id, shape, transform, areaType, tilePosition, locked->mTiles))
                {
                    newTiles.insert(tilePosition);
                    addChangedTile(tilePosition, ChangeType::add);
                    changed = true;
                }
            };
            getTilesPositions(range, onTilePosition);
            for (const auto& tile : data.mTiles)
            {
                if (newTiles.find(tile) == newTiles.end() && removeTile(id, tile, locked->mTiles))
                {
                    addChangedTile(tile, ChangeType::remove);
                    changed = true;
                }
            }
        }
        if (changed)
        {
            data.mTiles = std::move(newTiles);
            ++mRevision;
        }
        return changed;
    }

    void TileCachedRecastMeshManager::removeObject(const ObjectId id)
    {
        const auto object = mObjects.find(id);
        if (object == mObjects.end())
            return;
        bool changed = false;
        {
            const auto locked = mWorldspaceTiles.lock();
            for (const auto& tilePosition : object->second.mTiles)
            {
                if (removeTile(id, tilePosition, locked->mTiles))
                {
                    addChangedTile(tilePosition, ChangeType::remove);
                    changed = true;
                }
            }
        }
        mObjects.erase(object);
        if (changed)
            ++mRevision;
    }

    void TileCachedRecastMeshManager::addWater(const osg::Vec2i& cellPosition, const int cellSize, const float level)
    {
        const auto it = mWaterTilesPositions.find(cellPosition);
        if (it != mWaterTilesPositions.end())
            return;

        std::vector<TilePosition>& tilesPositions = mWaterTilesPositions.emplace_hint(
            it, cellPosition, std::vector<TilePosition>())->second;

        bool changed = false;

        if (cellSize == std::numeric_limits<int>::max())
        {
            const auto locked = mWorldspaceTiles.lock();
            for (auto& [tilePosition, data] : locked->mTiles)
            {
                if (data->addWater(cellPosition, cellSize, level))
                {
                    tilesPositions.push_back(tilePosition);
                    addChangedTile(tilePosition, ChangeType::add);
                    changed = true;
                }
            }
        }
        else
        {
            const btVector3 shift = Misc::Convert::toBullet(getWaterShift3d(cellPosition, cellSize, level));
            const auto worldspaceTiles = mWorldspaceTiles.lock();
            getTilesPositions(makeTilesPositionsRange(cellSize, shift, mSettings),
                [&] (const TilePosition& tilePosition)
                {
                    auto tile = worldspaceTiles->mTiles.find(tilePosition);
                    if (tile == worldspaceTiles->mTiles.end())
                    {
                        const TileBounds tileBounds = makeRealTileBoundsWithBorder(mSettings, tilePosition);
                        tile = worldspaceTiles->mTiles.emplace_hint(tile, tilePosition,
                                std::make_shared<CachedRecastMeshManager>(tileBounds, mTilesGeneration));
                    }
                    if (tile->second->addWater(cellPosition, cellSize, level))
                    {
                        tilesPositions.push_back(tilePosition);
                        addChangedTile(tilePosition, ChangeType::add);
                        changed = true;
                    }
                });
        }

        if (changed)
            ++mRevision;
    }

    void TileCachedRecastMeshManager::removeWater(const osg::Vec2i& cellPosition)
    {
        const auto object = mWaterTilesPositions.find(cellPosition);
        if (object == mWaterTilesPositions.end())
            return;
        bool changed = false;
        {
            const auto worldspaceTiles = mWorldspaceTiles.lock();
            for (const auto& tilePosition : object->second)
            {
                const auto tile = worldspaceTiles->mTiles.find(tilePosition);
                if (tile == worldspaceTiles->mTiles.end())
                    continue;
                if (tile->second->removeWater(cellPosition))
                {
                    addChangedTile(tilePosition, ChangeType::remove);
                    changed = true;
                }
                if (tile->second->isEmpty())
                {
                    worldspaceTiles->mTiles.erase(tile);
                    ++mTilesGeneration;
                }
            }
        }
        mWaterTilesPositions.erase(object);
        if (changed)
            ++mRevision;
    }

    void TileCachedRecastMeshManager::addHeightfield(const osg::Vec2i& cellPosition, const int cellSize,
        const HeightfieldShape& shape)
    {
        const auto it = mHeightfieldTilesPositions.find(cellPosition);
        if (it != mHeightfieldTilesPositions.end())
            return;

        std::vector<TilePosition>& tilesPositions = mHeightfieldTilesPositions.emplace_hint(
            it, cellPosition, std::vector<TilePosition>())->second;
        const btVector3 shift = getHeightfieldShift(shape, cellPosition, cellSize);

        bool changed = false;

        {
            const auto worldspaceTiles = mWorldspaceTiles.lock();
            getTilesPositions(makeTilesPositionsRange(cellSize, shift, mSettings),
                [&] (const TilePosition& tilePosition)
                {
                    auto tile = worldspaceTiles->mTiles.find(tilePosition);
                    if (tile == worldspaceTiles->mTiles.end())
                    {
                        const TileBounds tileBounds = makeRealTileBoundsWithBorder(mSettings, tilePosition);
                        tile = worldspaceTiles->mTiles.emplace_hint(tile, tilePosition,
                                std::make_shared<CachedRecastMeshManager>(tileBounds, mTilesGeneration));
                    }
                    if (tile->second->addHeightfield(cellPosition, cellSize, shape))
                    {
                        tilesPositions.push_back(tilePosition);
                        addChangedTile(tilePosition, ChangeType::add);
                        changed = true;
                    }
                });
        }

        if (changed)
            ++mRevision;
    }

    void TileCachedRecastMeshManager::removeHeightfield(const osg::Vec2i& cellPosition)
    {
        const auto object = mHeightfieldTilesPositions.find(cellPosition);
        if (object == mHeightfieldTilesPositions.end())
            return;
        bool changed = false;
        {
            const auto worldspaceTiles = mWorldspaceTiles.lock();
            for (const auto& tilePosition : object->second)
            {
                const auto tile = worldspaceTiles->mTiles.find(tilePosition);
                if (tile == worldspaceTiles->mTiles.end())
                    continue;
                if (tile->second->removeHeightfield(cellPosition))
                {
                    addChangedTile(tilePosition, ChangeType::remove);
                    changed = true;
                }
                if (tile->second->isEmpty())
                {
                    worldspaceTiles->mTiles.erase(tile);
                    ++mTilesGeneration;
                }
            }
        }
        mHeightfieldTilesPositions.erase(object);
        if (changed)
            ++mRevision;
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

    void TileCachedRecastMeshManager::reportNavMeshChange(const TilePosition& tilePosition, Version recastMeshVersion, Version navMeshVersion) const
    {
        const auto locked = mWorldspaceTiles.lockConst();
        const auto it = locked->mTiles.find(tilePosition);
        if (it == locked->mTiles.end())
            return;
        it->second->reportNavMeshChange(recastMeshVersion, navMeshVersion);
    }

    void TileCachedRecastMeshManager::addChangedTile(const TilePosition& tilePosition, ChangeType changeType)
    {
        auto tile = mChangedTiles.find(tilePosition);
        if (tile == mChangedTiles.end())
            mChangedTiles.emplace(tilePosition, changeType);
        else
            tile->second = addChangeType(tile->second, changeType);
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

    bool TileCachedRecastMeshManager::removeTile(const ObjectId id,
        const TilePosition& tilePosition, TilesMap& tiles)
    {
        const auto tile = tiles.find(tilePosition);
        if (tile == tiles.end())
            return false;
        const bool result = tile->second->removeObject(id);
        if (tile->second->isEmpty())
        {
            tiles.erase(tile);
            ++mTilesGeneration;
        }
        return result;
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
