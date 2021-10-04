#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_MAKENAVMESH_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_MAKENAVMESH_H

#include "offmeshconnectionsmanager.hpp"
#include "navmeshcacheitem.hpp"
#include "tileposition.hpp"
#include "sharednavmesh.hpp"
#include "navmeshtilescache.hpp"
#include "offmeshconnection.hpp"

#include <osg/Vec3f>

#include <memory>
#include <vector>

class dtNavMesh;

namespace DetourNavigator
{
    class RecastMesh;
    struct Settings;
    struct PreparedNavMeshData;
    struct NavMeshData;

    inline float getLength(const osg::Vec2i& value)
    {
        return std::sqrt(float(osg::square(value.x()) + osg::square(value.y())));
    }

    inline float getDistance(const TilePosition& lhs, const TilePosition& rhs)
    {
        return getLength(lhs - rhs);
    }

    inline bool shouldAddTile(const TilePosition& changedTile, const TilePosition& playerTile, int maxTiles)
    {
        const auto expectedTilesCount = std::ceil(osg::PI * osg::square(getDistance(changedTile, playerTile)));
        return expectedTilesCount <= maxTiles;
    }

    std::unique_ptr<PreparedNavMeshData> prepareNavMeshTileData(const RecastMesh& recastMesh, const TilePosition& tile,
        const Bounds& bounds, const osg::Vec3f& agentHalfExtents, const Settings& settings);

    NavMeshData makeNavMeshTileData(const PreparedNavMeshData& data,
        const std::vector<OffMeshConnection>& offMeshConnections, const osg::Vec3f& agentHalfExtents,
        const TilePosition& tile, const Settings& settings);

    NavMeshPtr makeEmptyNavMesh(const Settings& settings);

    enum class UpdateType
    {
        Persistent,
        Temporary
    };

    UpdateNavMeshStatus updateNavMesh(const osg::Vec3f& agentHalfExtents, const RecastMesh* recastMesh,
        const TilePosition& changedTile, const TilePosition& playerTile,
        const std::vector<OffMeshConnection>& offMeshConnections, const Settings& settings,
        const SharedNavMeshCacheItem& navMeshCacheItem, NavMeshTilesCache& navMeshTilesCache, UpdateType updateType);
}

#endif
