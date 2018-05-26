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

namespace DetourNavigator
{
    NavMeshManager::NavMeshManager(const Settings& settings)
        : mSettings(settings)
        , mRecastMeshManager(settings)
        , mAsyncNavMeshUpdater(settings, mRecastMeshManager)
    {
    }

    bool NavMeshManager::addObject(std::size_t id, const btCollisionShape& shape, const btTransform& transform)
    {
        if (!mRecastMeshManager.addObject(id, shape, transform))
            return false;
        addChangedTiles(shape, transform);
        return true;
    }

    bool NavMeshManager::updateObject(std::size_t id, const btCollisionShape& shape, const btTransform& transform)
    {
        if (!mRecastMeshManager.updateObject(id, transform))
            return false;
        addChangedTiles(shape, transform);
        return true;
    }

    bool NavMeshManager::removeObject(std::size_t id)
    {
        const auto object = mRecastMeshManager.removeObject(id);
        if (!object)
            return false;
        addChangedTiles(object->mShape, object->mTransform);
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

    void NavMeshManager::update(osg::Vec3f playerPosition, const osg::Vec3f& agentHalfExtents)
    {
        playerPosition *= mSettings.mRecastScaleFactor;
        std::swap(playerPosition.y(), playerPosition.z());
        const auto playerTile = getTilePosition(mSettings, playerPosition);
        if (mLastRecastMeshManagerRevision >= mRecastMeshManager.getRevision() && mPlayerTile
                && *mPlayerTile == playerTile)
            return;
        mLastRecastMeshManagerRevision = mRecastMeshManager.getRevision();
        mPlayerTile = playerTile;
        std::set<TilePosition> tilesToPost;
        const auto& cached = getCached(agentHalfExtents);
        const auto changedTiles = mChangedTiles.find(agentHalfExtents);
        {
            const auto locked = cached->mValue.lock();
            if (changedTiles != mChangedTiles.end())
            {
                for (const auto& tile : changedTiles->second)
                    if (locked->getTileAt(tile.x(), tile.y(), 0))
                        tilesToPost.insert(tile);
                for (const auto& tile : tilesToPost)
                    changedTiles->second.erase(tile);
                if (changedTiles->second.empty())
                    mChangedTiles.erase(changedTiles);
            }
            const auto maxTiles = locked->getParams()->maxTiles;
            mRecastMeshManager.forEachTilePosition([&] (const TilePosition& tile)
            {
                if (tilesToPost.count(tile))
                    return;
                const auto shouldAdd = shouldAddTile(tile, playerTile, maxTiles);
                const auto presentInNavMesh = bool(locked->getTileAt(tile.x(), tile.y(), 0));
                if ((shouldAdd && !presentInNavMesh) || (!shouldAdd && presentInNavMesh))
                    tilesToPost.insert(tile);
            });
        }
        mAsyncNavMeshUpdater.post(agentHalfExtents, cached, playerTile, tilesToPost);
        log("cache update posted for agent=", agentHalfExtents,
            " playerTile=", *mPlayerTile,
            " recastMeshManagerRevision=", mLastRecastMeshManagerRevision,
            " changedTiles=", changedTiles->second.size());
    }

    void NavMeshManager::wait()
    {
        mAsyncNavMeshUpdater.wait();
    }

    SharedNavMesh NavMeshManager::getNavMesh(const osg::Vec3f& agentHalfExtents) const
    {
        return getCached(agentHalfExtents)->mValue;
    }

    std::map<osg::Vec3f, std::shared_ptr<NavMeshCacheItem>> NavMeshManager::getNavMeshes() const
    {
        return mCache;
    }

    void NavMeshManager::addChangedTiles(const btCollisionShape& shape, const btTransform& transform)
    {
        getTilesPositions(shape, transform, mSettings, [&] (const TilePosition& v) {
            for (const auto& cached : mCache)
                if (cached.second)
                    mChangedTiles[cached.first].insert(v);
        });
    }

    const std::shared_ptr<NavMeshCacheItem>& NavMeshManager::getCached(const osg::Vec3f& agentHalfExtents) const
    {
        const auto cached = mCache.find(agentHalfExtents);
        if (cached != mCache.end())
            return cached->second;
        std::ostringstream stream;
        stream << "Agent with half extents is not found: " << agentHalfExtents;
        throw InvalidArgument(stream.str());
    }
}
