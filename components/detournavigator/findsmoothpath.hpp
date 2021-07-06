#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_FINDSMOOTHPATH_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_FINDSMOOTHPATH_H

#include "dtstatus.hpp"
#include "exceptions.hpp"
#include "flags.hpp"
#include "settings.hpp"
#include "settingsutils.hpp"
#include "debug.hpp"
#include "status.hpp"
#include "areatype.hpp"

#include <DetourCommon.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>

#include <osg/Vec3f>

#include <cassert>
#include <vector>

class dtNavMesh;

namespace DetourNavigator
{
    struct Settings;

    inline bool inRange(const osg::Vec3f& v1, const osg::Vec3f& v2, const float r)
    {
        return (osg::Vec2f(v1.x(), v1.z()) - osg::Vec2f(v2.x(), v2.z())).length() < r;
    }

    std::vector<dtPolyRef> fixupCorridor(const std::vector<dtPolyRef>& path, const std::vector<dtPolyRef>& visited);

    // This function checks if the path has a small U-turn, that is,
    // a polygon further in the path is adjacent to the first polygon
    // in the path. If that happens, a shortcut is taken.
    // This can happen if the target (T) location is at tile boundary,
    // and we're (S) approaching it parallel to the tile edge.
    // The choice at the vertex can be arbitrary,
    //  +---+---+
    //  |:::|:::|
    //  +-S-+-T-+
    //  |:::|   | <-- the step can end up in here, resulting U-turn path.
    //  +---+---+
    std::vector<dtPolyRef> fixupShortcuts(const std::vector<dtPolyRef>& path, const dtNavMeshQuery& navQuery);

    struct SteerTarget
    {
        osg::Vec3f steerPos;
        unsigned char steerPosFlag;
        dtPolyRef steerPosRef;
    };

    std::optional<SteerTarget> getSteerTarget(const dtNavMeshQuery& navQuery, const osg::Vec3f& startPos,
            const osg::Vec3f& endPos, const float minTargetDist, const std::vector<dtPolyRef>& path);

    template <class OutputIterator>
    class OutputTransformIterator
    {
    public:
        OutputTransformIterator(OutputIterator& impl, const Settings& settings)
            : mImpl(impl), mSettings(settings)
        {
        }

        OutputTransformIterator& operator *()
        {
            return *this;
        }

        OutputTransformIterator& operator ++()
        {
            ++mImpl.get();
            return *this;
        }

        OutputTransformIterator operator ++(int)
        {
            const auto copy = *this;
            ++(*this);
            return copy;
        }

        OutputTransformIterator& operator =(const osg::Vec3f& value)
        {
            *mImpl.get() = fromNavMeshCoordinates(mSettings, value);
            return *this;
        }

    private:
        std::reference_wrapper<OutputIterator> mImpl;
        std::reference_wrapper<const Settings> mSettings;
    };

    inline bool initNavMeshQuery(dtNavMeshQuery& value, const dtNavMesh& navMesh, const int maxNodes)
    {
        const auto status = value.init(&navMesh, maxNodes);
        return dtStatusSucceed(status);
    }

    dtPolyRef findNearestPolyExpanding(const dtNavMeshQuery& query, const dtQueryFilter& filter,
            const osg::Vec3f& center, const osg::Vec3f& halfExtents);

    struct MoveAlongSurfaceResult
    {
        osg::Vec3f mResultPos;
        std::vector<dtPolyRef> mVisited;
    };

    inline std::optional<MoveAlongSurfaceResult> moveAlongSurface(const dtNavMeshQuery& navMeshQuery,
        const dtPolyRef startRef, const osg::Vec3f& startPos, const osg::Vec3f& endPos, const dtQueryFilter& filter,
        const std::size_t maxVisitedSize)
    {
        MoveAlongSurfaceResult result;
        result.mVisited.resize(maxVisitedSize);
        int visitedNumber = 0;
        const auto status = navMeshQuery.moveAlongSurface(startRef, startPos.ptr(), endPos.ptr(),
            &filter, result.mResultPos.ptr(), result.mVisited.data(), &visitedNumber, static_cast<int>(maxVisitedSize));
        if (!dtStatusSucceed(status))
            return {};
        assert(visitedNumber >= 0);
        assert(visitedNumber <= static_cast<int>(maxVisitedSize));
        result.mVisited.resize(static_cast<std::size_t>(visitedNumber));
        return {std::move(result)};
    }

    inline std::optional<std::vector<dtPolyRef>> findPath(const dtNavMeshQuery& navMeshQuery, const dtPolyRef startRef,
        const dtPolyRef endRef, const osg::Vec3f& startPos, const osg::Vec3f& endPos, const dtQueryFilter& queryFilter,
        const std::size_t maxSize)
    {
        int pathLen = 0;
        std::vector<dtPolyRef> result(maxSize);
        const auto status = navMeshQuery.findPath(startRef, endRef, startPos.ptr(), endPos.ptr(), &queryFilter,
            result.data(), &pathLen, static_cast<int>(maxSize));
        if (!dtStatusSucceed(status))
            return {};
        assert(pathLen >= 0);
        assert(static_cast<std::size_t>(pathLen) <= maxSize);
        result.resize(static_cast<std::size_t>(pathLen));
        return {std::move(result)};
    }

    template <class OutputIterator>
    Status makeSmoothPath(const dtNavMesh& navMesh, const dtNavMeshQuery& navMeshQuery,
            const dtQueryFilter& filter, const osg::Vec3f& start, const osg::Vec3f& end, const float stepSize,
            std::vector<dtPolyRef> polygonPath, std::size_t maxSmoothPathSize, OutputIterator& out)
    {
        // Iterate over the path to find smooth path on the detail mesh surface.
        osg::Vec3f iterPos;
        navMeshQuery.closestPointOnPoly(polygonPath.front(), start.ptr(), iterPos.ptr(), nullptr);

        osg::Vec3f targetPos;
        navMeshQuery.closestPointOnPoly(polygonPath.back(), end.ptr(), targetPos.ptr(), nullptr);

        constexpr float slop = 0.01f;

        *out++ = iterPos;

        std::size_t smoothPathSize = 1;

        // Move towards target a small advancement at a time until target reached or
        // when ran out of memory to store the path.
        while (!polygonPath.empty() && smoothPathSize < maxSmoothPathSize)
        {
            // Find location to steer towards.
            const auto steerTarget = getSteerTarget(navMeshQuery, iterPos, targetPos, slop, polygonPath);

            if (!steerTarget)
                break;

            const bool endOfPath = bool(steerTarget->steerPosFlag & DT_STRAIGHTPATH_END);
            const bool offMeshConnection = bool(steerTarget->steerPosFlag & DT_STRAIGHTPATH_OFFMESH_CONNECTION);

            // Find movement delta.
            const osg::Vec3f delta = steerTarget->steerPos - iterPos;
            float len = delta.length();
            // If the steer target is end of path or off-mesh link, do not move past the location.
            if ((endOfPath || offMeshConnection) && len < stepSize)
                len = 1;
            else
                len = stepSize / len;

            const osg::Vec3f moveTgt = iterPos + delta * len;
            const auto result = moveAlongSurface(navMeshQuery, polygonPath.front(), iterPos, moveTgt, filter, 16);

            if (!result)
                return Status::MoveAlongSurfaceFailed;

            polygonPath = fixupCorridor(polygonPath, result->mVisited);
            polygonPath = fixupShortcuts(polygonPath, navMeshQuery);

            // Handle end of path and off-mesh links when close enough.
            if (endOfPath && inRange(result->mResultPos, steerTarget->steerPos, slop))
            {
                // Reached end of path.
                iterPos = targetPos;
                *out++ = iterPos;
                ++smoothPathSize;
                break;
            }
            else if (offMeshConnection && inRange(result->mResultPos, steerTarget->steerPos, slop))
            {
                // Advance the path up to and over the off-mesh connection.
                dtPolyRef prevRef = 0;
                dtPolyRef polyRef = polygonPath.front();
                std::size_t npos = 0;
                while (npos < polygonPath.size() && polyRef != steerTarget->steerPosRef)
                {
                    prevRef = polyRef;
                    polyRef = polygonPath[npos];
                    ++npos;
                }
                std::copy(polygonPath.begin() + std::ptrdiff_t(npos), polygonPath.end(), polygonPath.begin());
                polygonPath.resize(polygonPath.size() - npos);

                // Reached off-mesh connection.
                osg::Vec3f startPos;
                osg::Vec3f endPos;

                // Handle the connection.
                if (dtStatusSucceed(navMesh.getOffMeshConnectionPolyEndPoints(prevRef, polyRef,
                        startPos.ptr(), endPos.ptr())))
                {
                    *out++ = startPos;
                    ++smoothPathSize;

                    // Hack to make the dotted path not visible during off-mesh connection.
                    if (smoothPathSize & 1)
                    {
                        *out++ = startPos;
                        ++smoothPathSize;
                    }

                    // Move position at the other side of the off-mesh link.
                    if (dtStatusFailed(navMeshQuery.getPolyHeight(polygonPath.front(), endPos.ptr(), &iterPos.y())))
                        return Status::GetPolyHeightFailed;
                    iterPos.x() = endPos.x();
                    iterPos.z() = endPos.z();
                }
            }

            if (dtStatusFailed(navMeshQuery.getPolyHeight(polygonPath.front(), result->mResultPos.ptr(), &iterPos.y())))
                return Status::GetPolyHeightFailed;
            iterPos.x() = result->mResultPos.x();
            iterPos.z() = result->mResultPos.z();

            // Store results.
            *out++ = iterPos;
            ++smoothPathSize;
        }

        return Status::Success;
    }

    template <class OutputIterator>
    Status findSmoothPath(const dtNavMesh& navMesh, const osg::Vec3f& halfExtents, const float stepSize,
            const osg::Vec3f& start, const osg::Vec3f& end, const Flags includeFlags, const AreaCosts& areaCosts,
            const Settings& settings, OutputIterator& out)
    {
        dtNavMeshQuery navMeshQuery;
        if (!initNavMeshQuery(navMeshQuery, navMesh, settings.mMaxNavMeshQueryNodes))
            return Status::InitNavMeshQueryFailed;

        dtQueryFilter queryFilter;
        queryFilter.setIncludeFlags(includeFlags);
        queryFilter.setAreaCost(AreaType_water, areaCosts.mWater);
        queryFilter.setAreaCost(AreaType_door, areaCosts.mDoor);
        queryFilter.setAreaCost(AreaType_pathgrid, areaCosts.mPathgrid);
        queryFilter.setAreaCost(AreaType_ground, areaCosts.mGround);

        dtPolyRef startRef = findNearestPolyExpanding(navMeshQuery, queryFilter, start, halfExtents);
        if (startRef == 0)
            return Status::StartPolygonNotFound;

        dtPolyRef endRef = findNearestPolyExpanding(navMeshQuery, queryFilter, end, halfExtents);
        if (endRef == 0)
            return Status::EndPolygonNotFound;

        const auto polygonPath = findPath(navMeshQuery, startRef, endRef, start, end, queryFilter,
                                          settings.mMaxPolygonPathSize);

        if (!polygonPath)
            return Status::FindPathOverPolygonsFailed;

        if (polygonPath->empty() || polygonPath->back() != endRef)
            return Status::Success;

        auto outTransform = OutputTransformIterator<OutputIterator>(out, settings);
        return makeSmoothPath(navMesh, navMeshQuery, queryFilter, start, end, stepSize, std::move(*polygonPath),
            settings.mMaxSmoothPathSize, outTransform);
    }
}

#endif
