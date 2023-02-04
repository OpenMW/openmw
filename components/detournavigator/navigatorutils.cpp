#include "navigatorutils.hpp"
#include "debug.hpp"
#include "findrandompointaroundcircle.hpp"
#include "navigator.hpp"
#include "raycast.hpp"

#include <components/debug/debuglog.hpp>

namespace DetourNavigator
{
    std::optional<osg::Vec3f> findRandomPointAroundCircle(const Navigator& navigator, const AgentBounds& agentBounds,
        const osg::Vec3f& start, const float maxRadius, const Flags includeFlags, float (*prng)())
    {
        const auto navMesh = navigator.getNavMesh(agentBounds);
        if (!navMesh)
            return std::nullopt;
        const Settings& settings = navigator.getSettings();
        const auto locked = navMesh->lock();
        const auto result = DetourNavigator::findRandomPointAroundCircle(locked->getQuery(),
            toNavMeshCoordinates(settings.mRecast, agentBounds.mHalfExtents),
            toNavMeshCoordinates(settings.mRecast, start), toNavMeshCoordinates(settings.mRecast, maxRadius),
            includeFlags, prng);
        if (!result)
            return std::nullopt;
        return std::optional<osg::Vec3f>(fromNavMeshCoordinates(settings.mRecast, *result));
    }

    std::optional<osg::Vec3f> raycast(const Navigator& navigator, const AgentBounds& agentBounds,
        const osg::Vec3f& start, const osg::Vec3f& end, const Flags includeFlags)
    {
        const auto navMesh = navigator.getNavMesh(agentBounds);
        if (navMesh == nullptr)
            return std::nullopt;
        const Settings& settings = navigator.getSettings();
        const auto locked = navMesh->lock();
        const auto result = DetourNavigator::raycast(locked->getQuery(),
            toNavMeshCoordinates(settings.mRecast, agentBounds.mHalfExtents),
            toNavMeshCoordinates(settings.mRecast, start), toNavMeshCoordinates(settings.mRecast, end), includeFlags);
        if (!result)
            return std::nullopt;
        return fromNavMeshCoordinates(settings.mRecast, *result);
    }

    std::optional<osg::Vec3f> findNearestNavMeshPosition(const Navigator& navigator, const AgentBounds& agentBounds,
        const osg::Vec3f& position, const osg::Vec3f& searchAreaHalfExtents, const Flags includeFlags)
    {
        const auto navMesh = navigator.getNavMesh(agentBounds);
        if (navMesh == nullptr)
            return std::nullopt;

        const auto& settings = navigator.getSettings();
        const osg::Vec3f navMeshPosition = toNavMeshCoordinates(settings.mRecast, position);
        const auto lockedNavMesh = navMesh->lockConst();

        dtNavMeshQuery navMeshQuery;
        if (const dtStatus status
            = navMeshQuery.init(&lockedNavMesh->getImpl(), settings.mDetour.mMaxNavMeshQueryNodes);
            dtStatusFailed(status))
        {
            Log(Debug::Error) << "Failed to init dtNavMeshQuery for findNearestNavMeshPosition: "
                              << WriteDtStatus{ status };
            return std::nullopt;
        }

        dtQueryFilter queryFilter;
        queryFilter.setIncludeFlags(includeFlags);

        osg::Vec3f nearestNavMeshPos;
        const osg::Vec3f endPolyHalfExtents = toNavMeshCoordinates(settings.mRecast, searchAreaHalfExtents);
        dtPolyRef polyRef;
        if (const dtStatus status = navMeshQuery.findNearestPoly(
                navMeshPosition.ptr(), endPolyHalfExtents.ptr(), &queryFilter, &polyRef, nearestNavMeshPos.ptr());
            dtStatusFailed(status) || polyRef == 0)
        {
            return std::nullopt;
        }

        return fromNavMeshCoordinates(settings.mRecast, nearestNavMeshPos);
    }
}
