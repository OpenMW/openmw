#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_MAKENAVMESH_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_MAKENAVMESH_H

#include "settings.hpp"
#include "navmeshcacheitem.hpp"
#include "tileposition.hpp"

#include <osg/Vec3f>

#include <memory>
#include <set>

class dtNavMesh;

namespace DetourNavigator
{
    class RecastMesh;
    class SharedNavMesh;
    struct Settings;

    using NavMeshPtr = std::shared_ptr<dtNavMesh>;

    enum class UpdateNavMeshStatus
    {
        ignore,
        removed,
        add,
        replaced
    };

    NavMeshPtr makeEmptyNavMesh(const Settings& settings);

    UpdateNavMeshStatus updateNavMesh(const osg::Vec3f& agentHalfExtents, const RecastMesh& recastMesh,
            const TilePosition& changedTile, const Settings& settings, NavMeshCacheItem& navMeshCacheItem);
}

#endif
