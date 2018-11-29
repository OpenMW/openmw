#include "navmeshmanager.hpp"
#include "debug.hpp"
#include "exceptions.hpp"
#include "gettilespositions.hpp"
#include "makenavmesh.hpp"
#include "navmeshcacheitem.hpp"
#include "settings.hpp"
#include "sharednavmesh.hpp"

#include <DetourNavMesh.h>

#include <BulletCollision/CollisionShapes/btConcaveShape.h>

#include <iostream>

namespace
{
    using DetourNavigator::ChangeType;

    ChangeType addChangeType(const ChangeType current, const ChangeType add)
    {
        return current == add ? current : ChangeType::mixed;
    }
}

namespace DetourNavigator
{
    NavMeshManager::NavMeshManager(const Settings& settings)
        : mSettings(settings)
        , mRecastMeshManager(settings)
        , mOffMeshConnectionsManager(settings)
        , mAsyncNavMeshUpdater(settings, mRecastMeshManager, mOffMeshConnectionsManager)
    {}

    bool NavMeshManager::addObject(const ObjectId id, const btCollisionShape& shape, const btTransform& transform,
                                   const AreaType areaType)
    {
        if (!mRecastMeshManager.addObject(id, shape, transform, areaType))
            return false;
        addChangedTiles(shape, transform, ChangeType::add);
        return true;
    }

    bool NavMeshManager::updateObject(const ObjectId id, const btCollisionShape& shape, const btTransform& transform,
                                      const AreaType areaType)
    {
        if (!mRecastMeshManager.updateObject(id, transform, areaType))
            return false;
        addChangedTiles(shape, transform, ChangeType::update);
        return true;
    }

    bool NavMeshManager::removeObject(const ObjectId id)
    {
        const auto object = mRecastMeshManager.removeObject(id);
        if (!object)
            return false;
        addChangedTiles(object->mShape, object->mTransform, ChangeType::remove);
        return true;
    }

    bool NavMeshManager::addWater(const osg::Vec2i& cellPosition, const int cellSize, const btTransform& transform)
    {
        if (!mRecastMeshManager.addWater(cellPosition, cellSize, transform))
            return false;
        addChangedTiles(cellSize, transform, ChangeType::add);
        return true;
    }

    bool NavMeshManager::removeWater(const osg::Vec2i& cellPosition)
    {
        const auto water = mRecastMeshManager.removeWater(cellPosition);
        if (!water)
            return false;
        addChangedTiles(water->mCellSize, water->mTransform, ChangeType::remove);
        return true;
    }

    void NavMeshManager::addAgent(const osg::Vec3f& agentHalfExtents)
    {
        auto cached = mCache.find(agentHalfExtents);
        if (cached != mCache.end())
            return;
        mCache.insert(std::make_pair(agentHalfExtents,
            std::make_shared<NavMeshCacheItem>(makeEmptyNavMesh(mSettings), ++mGenerationCounter)));
        log("cache add for agent=", agentHalfExtents);
    }

    void NavMeshManager::reset(const osg::Vec3f& agentHalfExtents)
    {
        mCache.erase(agentHalfExtents);
    }

    void NavMeshManager::addOffMeshConnection(const ObjectId id, const osg::Vec3f& start, const osg::Vec3f& end)
    {
        if (!mOffMeshConnectionsManager.add(id, OffMeshConnection {start, end}))
            return;

        const auto startTilePosition = getTilePosition(mSettings, start);
        const auto endTilePosition = getTilePosition(mSettings, end);

        addChangedTile(startTilePosition, ChangeType::add);

        if (startTilePosition != endTilePosition)
            addChangedTile(endTilePosition, ChangeType::add);
    }

    void NavMeshManager::removeOffMeshConnection(const ObjectId id)
    {
        if (const auto connection = mOffMeshConnectionsManager.remove(id))
        {
            const auto startTilePosition = getTilePosition(mSettings, connection->mStart);
            const auto endTilePosition = getTilePosition(mSettings, connection->mEnd);

            addChangedTile(startTilePosition, ChangeType::remove);

            if (startTilePosition != endTilePosition)
                addChangedTile(endTilePosition, ChangeType::remove);
        }
    }

    void NavMeshManager::update(osg::Vec3f playerPosition, const osg::Vec3f& agentHalfExtents)
    {
        const auto playerTile = getTilePosition(mSettings, toNavMeshCoordinates(mSettings, playerPosition));
        auto& lastRevision = mLastRecastMeshManagerRevision[agentHalfExtents];
        auto lastPlayerTile = mPlayerTile.find(agentHalfExtents);
        if (lastRevision >= mRecastMeshManager.getRevision() && lastPlayerTile != mPlayerTile.end()
                && lastPlayerTile->second == playerTile)
            return;
        lastRevision = mRecastMeshManager.getRevision();
        if (lastPlayerTile == mPlayerTile.end())
            lastPlayerTile = mPlayerTile.insert(std::make_pair(agentHalfExtents, playerTile)).first;
        else
            lastPlayerTile->second = playerTile;
        std::map<TilePosition, ChangeType> tilesToPost;
        const auto& cached = getCached(agentHalfExtents);
        const auto changedTiles = mChangedTiles.find(agentHalfExtents);
        {
            const auto locked = cached.lock();
            const auto& navMesh = locked->getValue();
            if (changedTiles != mChangedTiles.end())
            {
                for (const auto& tile : changedTiles->second)
                    if (navMesh.getTileAt(tile.first.x(), tile.first.y(), 0))
                    {
                        auto tileToPost = tilesToPost.find(tile.first);
                        if (tileToPost == tilesToPost.end())
                            tilesToPost.insert(tile);
                        else
                            tileToPost->second = addChangeType(tileToPost->second, tile.second);
                    }
                for (const auto& tile : tilesToPost)
                    changedTiles->second.erase(tile.first);
                if (changedTiles->second.empty())
                    mChangedTiles.erase(changedTiles);
            }
            const auto maxTiles = navMesh.getParams()->maxTiles;
            mRecastMeshManager.forEachTilePosition([&] (const TilePosition& tile)
            {
                if (tilesToPost.count(tile))
                    return;
                const auto shouldAdd = shouldAddTile(tile, playerTile, maxTiles);
                const auto presentInNavMesh = bool(navMesh.getTileAt(tile.x(), tile.y(), 0));
                if (shouldAdd && !presentInNavMesh)
                    tilesToPost.insert(std::make_pair(tile, ChangeType::add));
                else if (!shouldAdd && presentInNavMesh)
                    tilesToPost.insert(std::make_pair(tile, ChangeType::mixed));
            });
        }
        mAsyncNavMeshUpdater.post(agentHalfExtents, cached, playerTile, tilesToPost);
        log("cache update posted for agent=", agentHalfExtents,
            " playerTile=", lastPlayerTile->second,
            " recastMeshManagerRevision=", lastRevision);
    }

    void NavMeshManager::wait()
    {
        mAsyncNavMeshUpdater.wait();
    }

    SharedNavMeshCacheItem NavMeshManager::getNavMesh(const osg::Vec3f& agentHalfExtents) const
    {
        return getCached(agentHalfExtents);
    }

    std::map<osg::Vec3f, SharedNavMeshCacheItem> NavMeshManager::getNavMeshes() const
    {
        return mCache;
    }

    void NavMeshManager::addChangedTiles(const btCollisionShape& shape, const btTransform& transform,
            const ChangeType changeType)
    {
        getTilesPositions(shape, transform, mSettings,
            [&] (const TilePosition& v) { addChangedTile(v, changeType); });
    }

    void NavMeshManager::addChangedTiles(const int cellSize, const btTransform& transform,
            const ChangeType changeType)
    {
        if (cellSize == std::numeric_limits<int>::max())
            return;

        getTilesPositions(cellSize, transform, mSettings,
            [&] (const TilePosition& v) { addChangedTile(v, changeType); });
    }

    void NavMeshManager::addChangedTile(const TilePosition& tilePosition, const ChangeType changeType)
    {
        for (const auto& cached : mCache)
        {
            auto& tiles = mChangedTiles[cached.first];
            auto tile = tiles.find(tilePosition);
            if (tile == tiles.end())
                tiles.insert(std::make_pair(tilePosition, changeType));
            else
                tile->second = addChangeType(tile->second, changeType);
        }
    }

    const SharedNavMeshCacheItem& NavMeshManager::getCached(const osg::Vec3f& agentHalfExtents) const
    {
        const auto cached = mCache.find(agentHalfExtents);
        if (cached != mCache.end())
            return cached->second;
        std::ostringstream stream;
        stream << "Agent with half extents is not found: " << agentHalfExtents;
        throw InvalidArgument(stream.str());
    }
}
