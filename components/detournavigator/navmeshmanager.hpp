#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHMANAGER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHMANAGER_H

#include "asyncnavmeshupdater.hpp"
#include "cachedrecastmeshmanager.hpp"

#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

#include <osg/Vec3f>

#include <map>
#include <memory>

class dtNavMesh;

namespace DetourNavigator
{
    using NavMeshConstPtr = std::shared_ptr<const dtNavMesh>;

    class NavMeshManager
    {
    public:
        NavMeshManager(const Settings& settings);

        template <class T>
        bool addObject(std::size_t id, const T& shape, const btTransform& transform)
        {
            if (!mRecastMeshManager.addObject(id, shape, transform))
                return false;
            ++mRevision;
            addChangedTiles(shape, transform);
            return true;
        }

        bool removeObject(std::size_t id);

        void reset(const osg::Vec3f& agentHalfExtents);

        void update(const osg::Vec3f& agentHalfExtents);

        void wait();

        NavMeshConstPtr getNavMesh(const osg::Vec3f& agentHalfExtents) const;

    private:
        std::size_t mRevision = 0;
        const Settings& mSettings;
        CachedRecastMeshManager mRecastMeshManager;
        std::map<osg::Vec3f, std::shared_ptr<NavMeshCacheItem>> mCache;
        std::map<osg::Vec3f, std::set<TilePosition>> mChangedTiles;
        AsyncNavMeshUpdater mAsyncNavMeshUpdater;

        void addChangedTiles(const btCollisionShape& shape, const btTransform& transform);
    };
}

#endif
