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
#include <functional>

class dtNavMesh;

namespace DetourNavigator
{
    struct Settings;

    inline bool inRange(const osg::Vec3f& v1, const osg::Vec3f& v2, const float r)
    {
        return (osg::Vec2f(v1.x(), v1.z()) - osg::Vec2f(v2.x(), v2.z())).length() < r;
    }

    std::size_t fixupCorridor(std::vector<dtPolyRef>& path, std::size_t pathSize, const std::vector<dtPolyRef>& visited);

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
    std::size_t fixupShortcuts(dtPolyRef* path, std::size_t pathSize, const dtNavMeshQuery& navQuery);

    struct SteerTarget
    {
        osg::Vec3f mSteerPos;
        unsigned char mSteerPosFlag;
        dtPolyRef mSteerPosRef;
    };

    std::optional<SteerTarget> getSteerTarget(const dtNavMeshQuery& navQuery, const osg::Vec3f& startPos,
            const osg::Vec3f& endPos, const float minTargetDist, const dtPolyRef* path, const std::size_t pathSize);

    template <class OutputIterator>
    class OutputTransformIterator
    {
    public:
        explicit OutputTransformIterator(OutputIterator& impl, const RecastSettings& settings)
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
        std::reference_wrapper<const RecastSettings> mSettings;
    };

    inline bool initNavMeshQuery(dtNavMeshQuery& value, const dtNavMesh& navMesh, const int maxNodes)
    {
        const auto status = value.init(&navMesh, maxNodes);
        return dtStatusSucceed(status);
    }

    dtPolyRef findNearestPoly(const dtNavMeshQuery& query, const dtQueryFilter& filter,
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

    inline std::optional<std::size_t> findPath(const dtNavMeshQuery& navMeshQuery, const dtPolyRef startRef,
        const dtPolyRef endRef, const osg::Vec3f& startPos, const osg::Vec3f& endPos, const dtQueryFilter& queryFilter,
        dtPolyRef* path, const std::size_t maxSize)
    {
        int pathLen = 0;
        const auto status = navMeshQuery.findPath(startRef, endRef, startPos.ptr(), endPos.ptr(), &queryFilter,
            path, &pathLen, static_cast<int>(maxSize));
        if (!dtStatusSucceed(status))
            return {};
        assert(pathLen >= 0);
        assert(static_cast<std::size_t>(pathLen) <= maxSize);
        return static_cast<std::size_t>(pathLen);
    }

    template <class OutputIterator>
    Status makeSmoothPath(const dtNavMesh& navMesh, const dtNavMeshQuery& navMeshQuery,
            const dtQueryFilter& filter, const osg::Vec3f& start, const osg::Vec3f& end, const float stepSize,
            std::vector<dtPolyRef>& polygonPath, std::size_t polygonPathSize, std::size_t maxSmoothPathSize, OutputIterator& out)
    {
        // Iterate over the path to find smooth path on the detail mesh surface.
        osg::Vec3f iterPos;
        navMeshQuery.closestPointOnPoly(polygonPath.front(), start.ptr(), iterPos.ptr(), nullptr);

        osg::Vec3f targetPos;
        navMeshQuery.closestPointOnPoly(polygonPath[polygonPathSize - 1], end.ptr(), targetPos.ptr(), nullptr);

        constexpr float slop = 0.01f;

        *out++ = iterPos;

        std::size_t smoothPathSize = 1;

        // Move towards target a small advancement at a time until target reached or
        // when ran out of memory to store the path.
        while (polygonPathSize > 0 && smoothPathSize < maxSmoothPathSize)
        {
            // Find location to steer towards.
            const auto steerTarget = getSteerTarget(navMeshQuery, iterPos, targetPos, slop, polygonPath.data(), polygonPathSize);

            if (!steerTarget)
                break;

            const bool endOfPath = bool(steerTarget->mSteerPosFlag & DT_STRAIGHTPATH_END);
            const bool offMeshConnection = bool(steerTarget->mSteerPosFlag & DT_STRAIGHTPATH_OFFMESH_CONNECTION);

            // Find movement delta.
            const osg::Vec3f delta = steerTarget->mSteerPos - iterPos;
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

            polygonPathSize = fixupCorridor(polygonPath, polygonPathSize, result->mVisited);
            polygonPathSize = fixupShortcuts(polygonPath.data(), polygonPathSize, navMeshQuery);

            // Handle end of path and off-mesh links when close enough.
            if (endOfPath && inRange(result->mResultPos, steerTarget->mSteerPos, slop))
            {
                // Reached end of path.
                iterPos = targetPos;
                *out++ = iterPos;
                ++smoothPathSize;
                break;
            }

            dtPolyRef polyRef = polygonPath.front();
            osg::Vec3f polyPos = result->mResultPos;

            if (offMeshConnection && inRange(polyPos, steerTarget->mSteerPos, slop))
            {
                // Advance the path up to and over the off-mesh connection.
                dtPolyRef prevRef = 0;
                std::size_t npos = 0;
                while (npos < polygonPathSize && polyRef != steerTarget->mSteerPosRef)
                {
                    prevRef = polyRef;
                    polyRef = polygonPath[npos];
                    ++npos;
                }
                if (npos > 0)
                {
                    std::copy(polygonPath.begin() + npos, polygonPath.begin() + polygonPathSize, polygonPath.begin());
                    polygonPathSize -= npos;
                }

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
                    polyPos = endPos;
                }
            }

            if (dtStatusFailed(navMeshQuery.getPolyHeight(polyRef, polyPos.ptr(), &iterPos.y())))
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
            const Settings& settings, float endTolerance, OutputIterator& out)
    {
        dtNavMeshQuery navMeshQuery;
        if (!initNavMeshQuery(navMeshQuery, navMesh, settings.mDetour.mMaxNavMeshQueryNodes))
            return Status::InitNavMeshQueryFailed;

        dtQueryFilter queryFilter;
        queryFilter.setIncludeFlags(includeFlags);
        queryFilter.setAreaCost(AreaType_water, areaCosts.mWater);
        queryFilter.setAreaCost(AreaType_door, areaCosts.mDoor);
        queryFilter.setAreaCost(AreaType_pathgrid, areaCosts.mPathgrid);
        queryFilter.setAreaCost(AreaType_ground, areaCosts.mGround);

        constexpr float polyDistanceFactor = 4;
        const osg::Vec3f polyHalfExtents = halfExtents * polyDistanceFactor;

        const dtPolyRef startRef = findNearestPoly(navMeshQuery, queryFilter, start, polyHalfExtents);
        if (startRef == 0)
            return Status::StartPolygonNotFound;

        const dtPolyRef endRef = findNearestPoly(navMeshQuery, queryFilter, end,
            polyHalfExtents + osg::Vec3f(endTolerance, endTolerance, endTolerance));
        if (endRef == 0)
            return Status::EndPolygonNotFound;

        std::vector<dtPolyRef> polygonPath(settings.mDetour.mMaxPolygonPathSize);
        const auto polygonPathSize = findPath(navMeshQuery, startRef, endRef, start, end, queryFilter,
                                              polygonPath.data(), polygonPath.size());

        if (!polygonPathSize.has_value())
            return Status::FindPathOverPolygonsFailed;

        if (*polygonPathSize == 0)
            return Status::Success;

        const bool partialPath = polygonPath[*polygonPathSize - 1] != endRef;
        auto outTransform = OutputTransformIterator<OutputIterator>(out, settings.mRecast);
        const Status smoothStatus = makeSmoothPath(navMesh, navMeshQuery, queryFilter, start, end, stepSize,
            polygonPath, *polygonPathSize, settings.mDetour.mMaxSmoothPathSize, outTransform);

        if (smoothStatus != Status::Success)
            return smoothStatus;

        return partialPath ? Status::PartialPath : Status::Success;
    }
}

#endif
