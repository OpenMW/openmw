#include "navigatorutils.hpp"
#include "findrandompointaroundcircle.hpp"
#include "navigator.hpp"
#include "raycast.hpp"

namespace DetourNavigator
{
    std::optional<osg::Vec3f> findRandomPointAroundCircle(const Navigator& navigator, const AgentBounds& agentBounds,
        const osg::Vec3f& start, const float maxRadius, const Flags includeFlags, float(*prng)())
    {
        const auto navMesh = navigator.getNavMesh(agentBounds);
        if (!navMesh)
            return std::nullopt;
        const auto& settings = navigator.getSettings();
        const auto result = DetourNavigator::findRandomPointAroundCircle(navMesh->lockConst()->getImpl(),
            toNavMeshCoordinates(settings.mRecast, agentBounds.mHalfExtents), toNavMeshCoordinates(settings.mRecast, start),
            toNavMeshCoordinates(settings.mRecast, maxRadius), includeFlags, settings.mDetour, prng);
        if (!result)
            return std::nullopt;
        return std::optional<osg::Vec3f>(fromNavMeshCoordinates(settings.mRecast, *result));
    }

    std::optional<osg::Vec3f> raycast(const Navigator& navigator, const AgentBounds& agentBounds, const osg::Vec3f& start,
        const osg::Vec3f& end, const Flags includeFlags)
    {
        const auto navMesh = navigator.getNavMesh(agentBounds);
        if (navMesh == nullptr)
            return std::nullopt;
        const auto& settings = navigator.getSettings();
        const auto result = DetourNavigator::raycast(navMesh->lockConst()->getImpl(),
            toNavMeshCoordinates(settings.mRecast, agentBounds.mHalfExtents), toNavMeshCoordinates(settings.mRecast, start),
            toNavMeshCoordinates(settings.mRecast, end), includeFlags, settings.mDetour);
        if (!result)
            return std::nullopt;
        return fromNavMeshCoordinates(settings.mRecast, *result);
    }
}
