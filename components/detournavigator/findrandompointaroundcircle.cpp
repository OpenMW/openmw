#include "findrandompointaroundcircle.hpp"
#include "settings.hpp"
#include "findsmoothpath.hpp"

#include <components/misc/rng.hpp>

#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>

namespace DetourNavigator
{
    std::optional<osg::Vec3f> findRandomPointAroundCircle(const dtNavMesh& navMesh, const osg::Vec3f& halfExtents,
        const osg::Vec3f& start, const float maxRadius, const Flags includeFlags, const DetourSettings& settings, float(*prng)())
    {
        dtNavMeshQuery navMeshQuery;
        if (!initNavMeshQuery(navMeshQuery, navMesh, settings.mMaxNavMeshQueryNodes))
            return std::optional<osg::Vec3f>();

        dtQueryFilter queryFilter;
        queryFilter.setIncludeFlags(includeFlags);

        dtPolyRef startRef = findNearestPoly(navMeshQuery, queryFilter, start, halfExtents * 4);
        if (startRef == 0)
            return std::optional<osg::Vec3f>();

        dtPolyRef resultRef = 0;
        osg::Vec3f resultPosition;

        navMeshQuery.findRandomPointAroundCircle(startRef, start.ptr(), maxRadius, &queryFilter,
            prng, &resultRef, resultPosition.ptr());

        if (resultRef == 0)
            return std::optional<osg::Vec3f>();

        return std::optional<osg::Vec3f>(resultPosition);
    }
}
