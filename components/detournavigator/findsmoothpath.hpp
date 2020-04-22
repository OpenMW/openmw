#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_FINDSMOOTHPATH_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_FINDSMOOTHPATH_H

#include "dtstatus.hpp"
#include "exceptions.hpp"
#include "flags.hpp"
#include "settings.hpp"
#include "settingsutils.hpp"
#include "debug.hpp"
#include "status.hpp"

#include <DetourCommon.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>

#include <LinearMath/btVector3.h>

#include <components/misc/convert.hpp>

#include <boost/optional.hpp>

#include <osg/Vec3f>

#include <vector>

class dtNavMesh;

namespace DetourNavigator
{
    struct Settings;

    inline bool inRange(const osg::Vec3f& v1, const osg::Vec3f& v2, const float r, const float h)
    {
        const auto d = v2 - v1;
        return (d.x() * d.x() + d.z() * d.z()) < r * r && std::abs(d.y()) < h;
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

    boost::optional<SteerTarget> getSteerTarget(const dtNavMeshQuery& navQuery, const osg::Vec3f& startPos,
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

    struct MoveAlongSurfaceResult
    {
        osg::Vec3f mResultPos;
        std::vector<dtPolyRef> mVisited;
    };

    inline boost::optional<MoveAlongSurfaceResult> moveAlongSurface(const dtNavMeshQuery& navMeshQuery,
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

    inline boost::optional<std::vector<dtPolyRef>> findPath(const dtNavMeshQuery& navMeshQuery, const dtPolyRef startRef,
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

    inline boost::optional<float> getPolyHeight(const dtNavMeshQuery& navMeshQuery, const dtPolyRef ref, const osg::Vec3f& pos)
    {
        float result = 0.0f;
        const auto status = navMeshQuery.getPolyHeight(ref, pos.ptr(), &result);
        if (!dtStatusSucceed(status))
            return {};
        return result;
    }

    template <class OutputIterator>
    Status makeSmoothPath(const dtNavMesh& navMesh, const dtNavMeshQuery& navMeshQuery,
            const dtQueryFilter& filter, const osg::Vec3f& start, const osg::Vec3f& end, const float stepSize,
            std::vector<dtPolyRef> polygonPath, std::size_t maxSmoothPathSize, OutputIterator& out)
    {
        // Iterate over the path to find smooth path on the detail mesh surface.
        osg::Vec3f iterPos;
        navMeshQuery.closestPointOnPoly(polygonPath.front(), start.ptr(), iterPos.ptr(), 0);

        osg::Vec3f targetPos;
        navMeshQuery.closestPointOnPoly(polygonPath.back(), end.ptr(), targetPos.ptr(), 0);

        const float SLOP = 0.01f;

        *out++ = iterPos;

        std::size_t smoothPathSize = 1;

        // Move towards target a small advancement at a time until target reached or
        // when ran out of memory to store the path.
        while (!polygonPath.empty() && smoothPathSize < maxSmoothPathSize)
        {
            // Find location to steer towards.
            const auto steerTarget = getSteerTarget(navMeshQuery, iterPos, targetPos, SLOP, polygonPath);

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

            float h = 0;
            navMeshQuery.getPolyHeight(polygonPath.front(), result->mResultPos.ptr(), &h);
            iterPos = result->mResultPos;
            iterPos.y() = h;

            // Handle end of path and off-mesh links when close enough.
            if (endOfPath && inRange(iterPos, steerTarget->steerPos, SLOP, 1.0f))
            {
                // Reached end of path.
                iterPos = targetPos;
                *out++ = iterPos;
                ++smoothPathSize;
                break;
            }
            else if (offMeshConnection && inRange(iterPos, steerTarget->steerPos, SLOP, 1.0f))
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
                    iterPos = endPos;
                    const auto height = getPolyHeight(navMeshQuery, polygonPath.front(), iterPos);

                    if (!height)
                        return Status::GetPolyHeightFailed;

                    iterPos.y() = *height;
                }
            }

            // Store results.
            *out++ = iterPos;
            ++smoothPathSize;
        }

        return Status::Success;
    }

    template <class OutputIterator>
    Status findSmoothPath(const dtNavMesh& navMesh, const osg::Vec3f& halfExtents, const float stepSize,
            const osg::Vec3f& start, const osg::Vec3f& end, const Flags includeFlags,
            const Settings& settings, OutputIterator& out)
    {
        dtNavMeshQuery navMeshQuery;
        if (!initNavMeshQuery(navMeshQuery, navMesh, settings.mMaxNavMeshQueryNodes))
            return Status::InitNavMeshQueryFailed;

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
            return Status::StartPolygonNotFound;

        dtPolyRef endRef = 0;
        osg::Vec3f endPolygonPosition;
        for (int i = 0; i < 3; ++i)
        {
            const auto status = navMeshQuery.findNearestPoly(end.ptr(), (halfExtents * (1 << i)).ptr(), &queryFilter,
                &endRef, endPolygonPosition.ptr());
            if (!dtStatusFailed(status) && endRef != 0)
                break;
        }

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
