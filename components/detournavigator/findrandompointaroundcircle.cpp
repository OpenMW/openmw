#include "findrandompointaroundcircle.hpp"
#include "findsmoothpath.hpp"

#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>

namespace DetourNavigator
{
    std::optional<osg::Vec3f> findRandomPointAroundCircle(const dtNavMeshQuery& navMeshQuery,
        const osg::Vec3f& halfExtents, const osg::Vec3f& start, const float maxRadius, const Flags includeFlags,
        float (*prng)())
    {
        dtQueryFilter queryFilter;
        queryFilter.setIncludeFlags(includeFlags);

        dtPolyRef startRef = findNearestPoly(navMeshQuery, queryFilter, start, halfExtents * 4);
        if (startRef == 0)
            return std::optional<osg::Vec3f>();

        dtPolyRef resultRef = 0;
        osg::Vec3f resultPosition;

        navMeshQuery.findRandomPointAroundCircle(
            startRef, start.ptr(), maxRadius, &queryFilter, prng, &resultRef, resultPosition.ptr());

        if (resultRef == 0)
            return std::optional<osg::Vec3f>();

        return std::optional<osg::Vec3f>(resultPosition);
    }
}
