#include "findrandompointaroundcircle.hpp"
#include "settings.hpp"
#include "findsmoothpath.hpp"

#include <components/misc/rng.hpp>

#include <DetourCommon.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>

namespace DetourNavigator
{
    boost::optional<osg::Vec3f> findRandomPointAroundCircle(const dtNavMesh& navMesh, const osg::Vec3f& halfExtents,
        const osg::Vec3f& start, const float maxRadius, const Flags includeFlags, const Settings& settings)
    {
        dtNavMeshQuery navMeshQuery;
        if (!initNavMeshQuery(navMeshQuery, navMesh, settings.mMaxNavMeshQueryNodes))
            return boost::optional<osg::Vec3f>();

        dtQueryFilter queryFilter;
        queryFilter.setIncludeFlags(includeFlags);

        dtPolyRef startRef = 0;
        osg::Vec3f startPolygonPosition;
        for (int i = 0; i < 3; ++i)
        {
            const auto status = navMeshQuery.findNearestPoly(start.ptr(), (halfExtents * (1 << i)).ptr(), &queryFilter,
                &startRef, startPolygonPosition.ptr());
            if (!dtStatusFailed(status) && startRef != 0)
                break;
        }

        if (startRef == 0)
            return boost::optional<osg::Vec3f>();

        dtPolyRef resultRef = 0;
        osg::Vec3f resultPosition;
        navMeshQuery.findRandomPointAroundCircle(startRef, start.ptr(), maxRadius, &queryFilter,
            &Misc::Rng::rollProbability, &resultRef, resultPosition.ptr());

        if (resultRef == 0)
            return boost::optional<osg::Vec3f>();

        return boost::optional<osg::Vec3f>(resultPosition);
    }
}
