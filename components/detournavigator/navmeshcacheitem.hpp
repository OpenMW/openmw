#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHCACHEITEM_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHCACHEITEM_H

#include "sharednavmesh.hpp"

#include <atomic>

namespace DetourNavigator
{
    struct NavMeshCacheItem
    {
        SharedNavMesh mValue;
        std::size_t mGeneration;
        std::atomic_size_t mNavMeshRevision;

        NavMeshCacheItem(const NavMeshPtr& value, std::size_t generation)
            : mValue(value), mGeneration(generation), mNavMeshRevision(0) {}
    };
}

#endif
