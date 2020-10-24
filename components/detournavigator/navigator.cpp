#include "findrandompointaroundcircle.hpp"
#include "navigator.hpp"

namespace DetourNavigator
{
    std::optional<osg::Vec3f> Navigator::findRandomPointAroundCircle(const osg::Vec3f& agentHalfExtents,
        const osg::Vec3f& start, const float maxRadius, const Flags includeFlags) const
    {
        const auto navMesh = getNavMesh(agentHalfExtents);
        if (!navMesh)
            return std::optional<osg::Vec3f>();
        const auto settings = getSettings();
        const auto result = DetourNavigator::findRandomPointAroundCircle(navMesh->lockConst()->getImpl(),
            toNavMeshCoordinates(settings, agentHalfExtents), toNavMeshCoordinates(settings, start),
            toNavMeshCoordinates(settings, maxRadius), includeFlags, settings);
        if (!result)
            return std::optional<osg::Vec3f>();
        return std::optional<osg::Vec3f>(fromNavMeshCoordinates(settings, *result));
    }
}
