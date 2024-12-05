#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVIGATORUTILS_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVIGATORUTILS_H

#include "findsmoothpath.hpp"
#include "flags.hpp"
#include "navigator.hpp"
#include "navmeshcacheitem.hpp"
#include "settings.hpp"

#include <components/misc/guarded.hpp>

#include <iterator>
#include <optional>

namespace DetourNavigator
{
    /**
     * @brief findPath fills output iterator with points of scene surfaces to be used for actor to walk through.
     * @param agentBounds defines which navmesh to use.
     * @param start path from given point.
     * @param end path at given point.
     * @param includeFlags setup allowed navmesh areas.
     * @param out the beginning of the destination range.
     * @param endTolerance defines maximum allowed distance to end path point in addition to agentHalfExtents
     * @return Status.
     * Equal to out if no path is found.
     */
    inline Status findPath(const Navigator& navigator, const AgentBounds& agentBounds, const osg::Vec3f& start,
        const osg::Vec3f& end, const Flags includeFlags, const AreaCosts& areaCosts, float endTolerance,
        std::output_iterator<osg::Vec3f> auto out)
    {
        const auto navMesh = navigator.getNavMesh(agentBounds);
        if (navMesh == nullptr)
            return Status::NavMeshNotFound;
        const Settings& settings = navigator.getSettings();
        FromNavMeshCoordinatesIterator outTransform(out, settings.mRecast);
        const auto locked = navMesh->lock();
        return findSmoothPath(locked->getQuery(), toNavMeshCoordinates(settings.mRecast, agentBounds.mHalfExtents),
            toNavMeshCoordinates(settings.mRecast, start), toNavMeshCoordinates(settings.mRecast, end), includeFlags,
            areaCosts, settings.mDetour, endTolerance, outTransform);
    }

    /**
     * @brief findRandomPointAroundCircle returns random location on navmesh within the reach of specified location.
     * @param agentBounds defines which navmesh to use.
     * @param start is a position where the search starts.
     * @param maxRadius limit maximum distance from start.
     * @param includeFlags setup allowed navmesh areas.
     * @return not empty optional with position if point is found and empty optional if point is not found.
     */
    std::optional<osg::Vec3f> findRandomPointAroundCircle(const Navigator& navigator, const AgentBounds& agentBounds,
        const osg::Vec3f& start, const float maxRadius, const Flags includeFlags, float (*prng)());

    /**
     * @brief raycast finds farest navmesh point from start on a line from start to end that has path from start.
     * @param agentBounds defines which navmesh to use.
     * @param start of the line
     * @param end of the line
     * @param includeFlags setup allowed navmesh areas.
     * @return not empty optional with position if point is found and empty optional if point is not found.
     */
    std::optional<osg::Vec3f> raycast(const Navigator& navigator, const AgentBounds& agentBounds,
        const osg::Vec3f& start, const osg::Vec3f& end, const Flags includeFlags);

    /**
     * @brief findNearestNavMeshPosition finds nearest position on navmesh within given area having given flags.
     * @param agentBounds defines which navmesh to use.
     * @param position is a center of the search area.
     * @param searchAreaHalfExtents defines AABB like area around given postion.
     * @param includeFlags setup allowed navmesh areas.
     * @return not empty optional with position if position is found and empty optional if position is not found.
     */
    std::optional<osg::Vec3f> findNearestNavMeshPosition(const Navigator& navigator, const AgentBounds& agentBounds,
        const osg::Vec3f& position, const osg::Vec3f& searchAreaHalfExtents, const Flags includeFlags);
}

#endif
