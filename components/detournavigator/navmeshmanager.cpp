#include "navmeshmanager.hpp"
#include "debug.hpp"
#include "exceptions.hpp"
#include "gettilespositions.hpp"
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

    bool NavMeshManager::addObject(std::size_t id, const btCollisionShape& shape, const btTransform& transform)
    {
        if (!mRecastMeshManager.addObject(id, shape, transform))
            return false;
        ++mRevision;
        addChangedTiles(shape, transform);
        return true;
    }

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
            std::make_shared<NavMeshCacheItem>(makeEmptyNavMesh(mSettings), ++mGenerationCounter, mRevision))
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
        if (cached->mRecastMeshRevision >= mRevision)
            return;
        cached->mRecastMeshRevision = mRevision;
        const auto changedTiles = mChangedTiles.find(agentHalfExtents);
        if (changedTiles != mChangedTiles.end())
        {
            playerPosition *= mSettings.mRecastScaleFactor;
            std::swap(playerPosition.y(), playerPosition.z());
            mAsyncNavMeshUpdater.post(agentHalfExtents, mRecastMeshManager.getMesh(), cached,
                                      getTilePosition(mSettings, playerPosition), changedTiles->second);
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
