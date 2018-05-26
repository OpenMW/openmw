#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_CACHEDRECASTMESHMANAGER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_CACHEDRECASTMESHMANAGER_H

#include "recastmeshmanager.hpp"

#include <boost/optional.hpp>

namespace DetourNavigator
{
    class CachedRecastMeshManager
    {
    public:
        CachedRecastMeshManager(const Settings& settings, const TileBounds& bounds);

        bool addObject(std::size_t id, const btCollisionShape& shape, const btTransform& transform);

        bool updateObject(std::size_t id, const btTransform& transform);

        boost::optional<RemovedRecastMeshObject> removeObject(std::size_t id);

        std::shared_ptr<RecastMesh> getMesh();

        bool isEmpty() const;

    private:
        RecastMeshManager mImpl;
        std::shared_ptr<RecastMesh> mCached;
    };
}

#endif
