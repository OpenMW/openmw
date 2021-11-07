#include "navigatorutils.hpp"
#include "findrandompointaroundcircle.hpp"
#include "navigator.hpp"
#include "raycast.hpp"

namespace DetourNavigator
{
    std::optional<osg::Vec3f> findRandomPointAroundCircle(const Navigator& navigator, const osg::Vec3f& agentHalfExtents,
        const osg::Vec3f& start, const float maxRadius, const Flags includeFlags)
    {
        const auto navMesh = navigator.getNavMesh(agentHalfExtents);
        if (!navMesh)
            return std::nullopt;
        const auto settings = navigator.getSettings();
        const auto result = DetourNavigator::findRandomPointAroundCircle(navMesh->lockConst()->getImpl(),
            toNavMeshCoordinates(settings, agentHalfExtents), toNavMeshCoordinates(settings, start),
            toNavMeshCoordinates(settings, maxRadius), includeFlags, settings);
        if (!result)
            return std::nullopt;
        return std::optional<osg::Vec3f>(fromNavMeshCoordinates(settings, *result));
    }

    std::optional<osg::Vec3f> raycast(const Navigator& navigator, const osg::Vec3f& agentHalfExtents, const osg::Vec3f& start,
        const osg::Vec3f& end, const Flags includeFlags)
    {
        const auto navMesh = navigator.getNavMesh(agentHalfExtents);
        if (navMesh == nullptr)
            return std::nullopt;
        const auto settings = navigator.getSettings();
        const auto result = DetourNavigator::raycast(navMesh->lockConst()->getImpl(),
            toNavMeshCoordinates(settings, agentHalfExtents), toNavMeshCoordinates(settings, start),
            toNavMeshCoordinates(settings, end), includeFlags, settings);
        if (!result)
            return std::nullopt;
        return fromNavMeshCoordinates(settings, *result);
    }
}
