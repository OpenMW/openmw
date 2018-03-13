#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHMANAGER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHMANAGER_H

#include "asyncnavmeshupdater.hpp"
#include "cachedrecastmeshmanager.hpp"

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

        template <class T>
        bool addObject(std::size_t id, const T& shape, const btTransform& transform)
        {
            if (!mRecastMeshManager.addObject(id, shape, transform))
                return false;
            ++mRevision;
            return true;
        }

        bool removeObject(std::size_t id);

        void reset(const osg::Vec3f& agentHalfExtents);

        void update(const osg::Vec3f& agentHalfExtents);

        void wait();

        NavMeshConstPtr getNavMesh(const osg::Vec3f& agentHalfExtents) const;

    private:
        std::size_t mRevision = 0;
        CachedRecastMeshManager mRecastMeshManager;
        std::map<osg::Vec3f, std::shared_ptr<NavMeshCacheItem>> mCache;
        AsyncNavMeshUpdater mAsyncNavMeshUpdater;
    };
}

#endif
