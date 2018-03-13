#include "navmeshmanager.hpp"
#include "debug.hpp"

#include <iostream>

namespace DetourNavigator
{
    NavMeshManager::NavMeshManager(const Settings& settings)
        : mRecastMeshManager(settings)
        , mAsyncNavMeshUpdater(settings)
    {
    }

    bool NavMeshManager::removeObject(std::size_t id)
    {
        if (!mRecastMeshManager.removeObject(id))
            return false;
        ++mRevision;
        return true;
    }

    void NavMeshManager::reset(const osg::Vec3f& agentHalfExtents)
    {
        mCache.erase(agentHalfExtents);
    }

    void NavMeshManager::update(const osg::Vec3f& agentHalfExtents)
    {
        auto it = mCache.find(agentHalfExtents);
        if (it == mCache.end())
            it = mCache.insert(std::make_pair(agentHalfExtents, std::make_shared<NavMeshCacheItem>(mRevision))).first;
        else if (it->second->mRevision >= mRevision)
            return;
        it->second->mRevision = mRevision;
        mAsyncNavMeshUpdater.post(agentHalfExtents, mRecastMeshManager.getMesh(), it->second);
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
}
