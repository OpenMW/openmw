#include "navigator.hpp"
#include "debug.hpp"
#include "settingsutils.hpp"

namespace DetourNavigator
{
    Navigator::Navigator(const Settings& settings)
        : mSettings(settings)
        , mNavMeshManager(mSettings)
    {
    }

    void Navigator::addAgent(const osg::Vec3f& agentHalfExtents)
    {
        ++mAgents[agentHalfExtents];
        mNavMeshManager.addAgent(agentHalfExtents);
    }

    void Navigator::removeAgent(const osg::Vec3f& agentHalfExtents)
    {
        const auto it = mAgents.find(agentHalfExtents);
        if (it == mAgents.end() || --it->second)
            return;
        mAgents.erase(it);
        mNavMeshManager.reset(agentHalfExtents);
    }

    bool Navigator::addObject(std::size_t id, const btCollisionShape& shape, const btTransform& transform)
    {
        return mNavMeshManager.addObject(id, shape, transform);
    }

    bool Navigator::removeObject(std::size_t id)
    {
        return mNavMeshManager.removeObject(id);
    }

    void Navigator::update(const osg::Vec3f& playerPosition)
    {
        for (const auto& v : mAgents)
            mNavMeshManager.update(playerPosition, v.first);
    }

    void Navigator::wait()
    {
        mNavMeshManager.wait();
    }

    std::map<osg::Vec3f, std::shared_ptr<NavMeshCacheItem>> Navigator::getNavMeshes() const
    {
        return mNavMeshManager.getNavMeshes();
    }

    const Settings& Navigator::getSettings() const
    {
        return mSettings;
    }
}
