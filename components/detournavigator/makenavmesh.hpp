#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_MAKENAVMESH_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_MAKENAVMESH_H

#include "offmeshconnectionsmanager.hpp"
#include "navmeshcacheitem.hpp"
#include "tileposition.hpp"
#include "sharednavmesh.hpp"
#include "navmeshtilescache.hpp"
#include "offmeshconnection.hpp"
#include "navmeshdb.hpp"

#include <components/misc/guarded.hpp>

#include <osg/Vec3f>

#include <memory>
#include <vector>

class dtNavMesh;
struct rcConfig;

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

    inline bool isEmpty(const RecastMesh& recastMesh)
    {
        return recastMesh.getMesh().getIndices().empty()
                && recastMesh.getWater().empty()
                && recastMesh.getHeightfields().empty()
                && recastMesh.getFlatHeightfields().empty();
    }

    std::unique_ptr<PreparedNavMeshData> prepareNavMeshTileData(const RecastMesh& recastMesh,
        const TilePosition& tilePosition, const osg::Vec3f& agentHalfExtents, const RecastSettings& settings);

    NavMeshData makeNavMeshTileData(const PreparedNavMeshData& data,
        const std::vector<OffMeshConnection>& offMeshConnections, const osg::Vec3f& agentHalfExtents,
        const TilePosition& tile, const RecastSettings& settings);

    NavMeshPtr makeEmptyNavMesh(const Settings& settings);
}

#endif
