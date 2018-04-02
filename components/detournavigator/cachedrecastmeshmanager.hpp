#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_CACHEDRECASTMESHMANAGER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_CACHEDRECASTMESHMANAGER_H

#include "recastmeshmanager.hpp"

#include <boost/optional.hpp>

namespace DetourNavigator
{
    class CachedRecastMeshManager
    {
    public:
        CachedRecastMeshManager(const Settings& settings);

        bool addObject(std::size_t id, const btCollisionShape& shape, const btTransform& transform);

        boost::optional<RecastMeshManager::Object> removeObject(std::size_t id);

        std::shared_ptr<RecastMesh> getMesh();

    private:
        RecastMeshManager mImpl;
        std::shared_ptr<RecastMesh> mCached;
    };
}

#endif
