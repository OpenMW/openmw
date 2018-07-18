#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHMANAGER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHMANAGER_H

#include "asyncnavmeshupdater.hpp"
#include "cachedrecastmeshmanager.hpp"
#include "sharednavmesh.hpp"

#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

#include <osg/Vec3f>

#include <map>
#include <memory>

class dtNavMesh;

namespace DetourNavigator
{
    class NavMeshManager
    {
    public:
        NavMeshManager(const Settings& settings);

        bool addObject(std::size_t id, const btCollisionShape& shape, const btTransform& transform,
                       const AreaType areaType);

        bool updateObject(std::size_t id, const btCollisionShape& shape, const btTransform& transform,
                          const AreaType areaType);

        bool removeObject(std::size_t id);

        void addAgent(const osg::Vec3f& agentHalfExtents);

        void reset(const osg::Vec3f& agentHalfExtents);

        void update(osg::Vec3f playerPosition, const osg::Vec3f& agentHalfExtents);

        void wait();

        SharedNavMesh getNavMesh(const osg::Vec3f& agentHalfExtents) const;

        std::map<osg::Vec3f, std::shared_ptr<NavMeshCacheItem>> getNavMeshes() const;

    private:
        const Settings& mSettings;
        TileCachedRecastMeshManager mRecastMeshManager;
        std::map<osg::Vec3f, std::shared_ptr<NavMeshCacheItem>> mCache;
        std::map<osg::Vec3f, std::map<TilePosition, ChangeType>> mChangedTiles;
        AsyncNavMeshUpdater mAsyncNavMeshUpdater;
        std::size_t mGenerationCounter = 0;
        boost::optional<TilePosition> mPlayerTile;
        std::size_t mLastRecastMeshManagerRevision = 0;

        void addChangedTiles(const btCollisionShape& shape, const btTransform& transform, const ChangeType changeType);

        const std::shared_ptr<NavMeshCacheItem>& getCached(const osg::Vec3f& agentHalfExtents) const;
    };
}

#endif
