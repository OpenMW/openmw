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
        std::size_t mRecastMeshRevision;
        std::atomic_size_t mNavMeshRevision;

        NavMeshCacheItem(const NavMeshPtr& value, std::size_t generation, std::size_t revision)
            : mValue(value), mGeneration(generation), mRecastMeshRevision(revision), mNavMeshRevision(0) {}
    };
}

#endif
