#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVIGATOR_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVIGATOR_H

#include "findsmoothpath.hpp"
#include "navmeshmanager.hpp"
#include "settings.hpp"
#include "settingsutils.hpp"

namespace DetourNavigator
{
    class Navigator
    {
    public:
        Navigator(const Settings& settings);

        void addAgent(const osg::Vec3f& agentHalfExtents);

        void removeAgent(const osg::Vec3f& agentHalfExtents);

        bool addObject(std::size_t id, const btCollisionShape& shape, const btTransform& transform);

        bool removeObject(std::size_t id);

        void update(const osg::Vec3f& playerPosition);

        void wait();

        template <class OutputIterator>
        OutputIterator findPath(const osg::Vec3f& agentHalfExtents, const osg::Vec3f& start,
            const osg::Vec3f& end, OutputIterator out) const
        {
            const auto navMesh = mNavMeshManager.getNavMesh(agentHalfExtents);
            return findSmoothPath(*navMesh.lock(), toNavMeshCoordinates(mSettings, agentHalfExtents),
                toNavMeshCoordinates(mSettings, start), toNavMeshCoordinates(mSettings, end), mSettings, out);
        }

        std::map<osg::Vec3f, std::shared_ptr<NavMeshCacheItem>> getNavMeshes() const;

        const Settings& getSettings() const;

    private:
        Settings mSettings;
        NavMeshManager mNavMeshManager;
        std::map<osg::Vec3f, std::size_t> mAgents;
    };
}

#endif
