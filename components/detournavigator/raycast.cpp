#include "raycast.hpp"
#include "settings.hpp"
#include "findsmoothpath.hpp"

#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>

#include <array>

namespace DetourNavigator
{
    std::optional<osg::Vec3f> raycast(const dtNavMesh& navMesh, const osg::Vec3f& halfExtents,
        const osg::Vec3f& start, const osg::Vec3f& end, const Flags includeFlags, const Settings& settings)
    {
        dtNavMeshQuery navMeshQuery;
        if (!initNavMeshQuery(navMeshQuery, navMesh, settings.mMaxNavMeshQueryNodes))
            return {};

        dtQueryFilter queryFilter;
        queryFilter.setIncludeFlags(includeFlags);

        dtPolyRef ref = 0;
        if (dtStatus status = navMeshQuery.findNearestPoly(start.ptr(), halfExtents.ptr(), &queryFilter, &ref, nullptr);
            dtStatusFailed(status) || ref == 0)
            return {};

        const unsigned options = 0;
        std::array<dtPolyRef, 16> path;
        dtRaycastHit hit;
        hit.path = path.data();
        hit.maxPath = path.size();
        if (dtStatus status = navMeshQuery.raycast(ref, start.ptr(), end.ptr(), &queryFilter, options, &hit);
            dtStatusFailed(status) || hit.pathCount == 0)
            return {};

        osg::Vec3f hitPosition;
        if (dtStatus status = navMeshQuery.closestPointOnPoly(path[hit.pathCount - 1], end.ptr(), hitPosition.ptr(), nullptr);
            dtStatusFailed(status))
            return {};

        return hitPosition;
    }
}
