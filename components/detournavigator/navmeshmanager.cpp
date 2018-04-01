#include "navmeshmanager.hpp"
#include "debug.hpp"
#include "exceptions.hpp"
#include "makenavmesh.hpp"
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
        , mAsyncNavMeshUpdater(settings)
    {}

    bool NavMeshManager::removeObject(std::size_t id)
    {
        const auto object = mRecastMeshManager.removeObject(id);
        if (!object)
            return false;
        ++mRevision;
        addChangedTiles(*object->mShape, object->mTransform);
        return true;
    }

    void NavMeshManager::addAgent(const osg::Vec3f& agentHalfExtents)
    {
        auto cached = mCache.find(agentHalfExtents);
        if (cached != mCache.end())
            return;
        mCache.insert(std::make_pair(agentHalfExtents,
            std::make_shared<NavMeshCacheItem>(NavMeshCacheItem {makeEmptyNavMesh(mSettings), mRevision}))
        );
        log("cache add for agent=", agentHalfExtents);
    }

    void NavMeshManager::reset(const osg::Vec3f& agentHalfExtents)
    {
        mCache.erase(agentHalfExtents);
    }

    void NavMeshManager::update(osg::Vec3f playerPosition, const osg::Vec3f& agentHalfExtents)
    {
        const auto& cached = getCached(agentHalfExtents);
        if (cached->mRevision >= mRevision)
            return;
        cached->mRevision = mRevision;
        const auto changedTiles = mChangedTiles.find(agentHalfExtents);
        if (changedTiles != mChangedTiles.end())
        {
            TilePosition playerTile;
            playerPosition *= mSettings.mRecastScaleFactor;
            std::swap(playerPosition.y(), playerPosition.z());
            cached->mValue.raw()->calcTileLoc(playerPosition.ptr(), &playerTile.x(), &playerTile.y());
            mAsyncNavMeshUpdater.post(agentHalfExtents, mRecastMeshManager.getMesh(), cached, playerTile,
                                      changedTiles->second);
            mChangedTiles.erase(changedTiles);
            log("cache update posted for agent=", agentHalfExtents);
        }
    }

    void NavMeshManager::wait()
    {
        mAsyncNavMeshUpdater.wait();
    }

    SharedNavMesh NavMeshManager::getNavMesh(const osg::Vec3f& agentHalfExtents) const
    {
        return getCached(agentHalfExtents)->mValue;
    }

    void NavMeshManager::addChangedTiles(const btCollisionShape& shape, const btTransform& transform)
    {
        btVector3 aabbMin;
        btVector3 aabbMax;
        shape.getAabb(transform, aabbMin, aabbMax);
        osg::Vec3f min(aabbMin.x(), aabbMin.z(), aabbMin.y());
        osg::Vec3f max(aabbMax.x(), aabbMax.z(), aabbMax.y());
        min *= mSettings.mRecastScaleFactor;
        max *= mSettings.mRecastScaleFactor;

        for (auto& v : mCache)
        {
            if (const auto& item = v.second)
            {
                auto& changedTiles = mChangedTiles[v.first];

                int minTileX;
                int minTileY;
                item->mValue.raw()->calcTileLoc(min.ptr(), &minTileX, &minTileY);

                int maxTileX;
                int maxTileY;
                item->mValue.raw()->calcTileLoc(max.ptr(), &maxTileX, &maxTileY);

                if (minTileX > maxTileX)
                    std::swap(minTileX, maxTileX);

                if (minTileY > maxTileY)
                    std::swap(minTileY, maxTileY);

                for (int tileX = minTileX; tileX <= maxTileX; ++tileX)
                    for (int tileY = minTileY; tileY <= maxTileY; ++tileY)
                        changedTiles.insert(TilePosition {tileX, tileY});
            }
        }
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
