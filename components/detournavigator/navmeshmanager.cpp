#include "navmeshmanager.hpp"
#include "debug.hpp"
#include "makenavmesh.hpp"
#include "settings.hpp"

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

    void NavMeshManager::reset(const osg::Vec3f& agentHalfExtents)
    {
        mCache.erase(agentHalfExtents);
    }

    void NavMeshManager::update(const osg::Vec3f& agentHalfExtents)
    {
        auto cached = mCache.find(agentHalfExtents);
        if (cached == mCache.end())
            cached = mCache.insert(std::make_pair(agentHalfExtents,
                std::make_shared<NavMeshCacheItem>(NavMeshCacheItem {
                    makeEmptyNavMesh(agentHalfExtents, *mRecastMeshManager.getMesh(), mSettings),
                    mRevision
                }))).first;
        else if (cached->second->mRevision >= mRevision)
            return;
        cached->second->mRevision = mRevision;
        const auto changedTiles = mChangedTiles.find(agentHalfExtents);
        if (changedTiles == mChangedTiles.end())
        {
            mAsyncNavMeshUpdater.post(agentHalfExtents, mRecastMeshManager.getMesh(), cached->second,
                                      std::set<TilePosition>());
        }
        else
        {
            mAsyncNavMeshUpdater.post(agentHalfExtents, mRecastMeshManager.getMesh(), cached->second,
                                      std::move(changedTiles->second));
            mChangedTiles.erase(changedTiles);
        }
    }

    void NavMeshManager::wait()
    {
        mAsyncNavMeshUpdater.wait();
    }

    NavMeshConstPtr NavMeshManager::getNavMesh(const osg::Vec3f& agentHalfExtents) const
    {
        const auto it = mCache.find(agentHalfExtents);
        if (it == mCache.end())
            return nullptr;
        return it->second->mValue;
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
                if (const auto& navMesh = item->mValue)
                {
                    auto& changedTiles = mChangedTiles[v.first];

                    int minTileX;
                    int minTileY;
                    navMesh->calcTileLoc(min.ptr(), &minTileX, &minTileY);

                    int maxTileX;
                    int maxTileY;
                    navMesh->calcTileLoc(max.ptr(), &maxTileX, &maxTileY);

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
    }
}
