#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_FINDSMOOTHPATH_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_FINDSMOOTHPATH_H

#include "areatype.hpp"
#include "flags.hpp"
#include "settings.hpp"
#include "settingsutils.hpp"
#include "status.hpp"

#include <DetourCommon.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>

#include <osg/Vec3f>

#include <array>
#include <cassert>
#include <functional>
#include <span>
#include <vector>

namespace DetourNavigator
{
    template <class OutputIterator, class Function>
    class OutputTransformIterator
    {
    public:
        explicit OutputTransformIterator(OutputIterator& impl, Function&& function)
            : mImpl(impl)
            , mFunction(std::forward<Function>(function))
        {
        }

        OutputTransformIterator& operator*() { return *this; }

        OutputTransformIterator& operator++()
        {
            ++mImpl.get();
            return *this;
        }

        OutputTransformIterator operator++(int)
        {
            const auto copy = *this;
            ++(*this);
            return copy;
        }

        OutputTransformIterator& operator=(const osg::Vec3f& value)
        {
            *mImpl.get() = mFunction(value);
            return *this;
        }

    private:
        std::reference_wrapper<OutputIterator> mImpl;
        Function mFunction;
    };

    template <class OutputIterator>
    auto withFromNavMeshCoordinates(OutputIterator& impl, const RecastSettings& settings)
    {
        return OutputTransformIterator(
            impl, [&settings](const osg::Vec3f& value) { return fromNavMeshCoordinates(settings, value); });
    }

    inline std::optional<std::size_t> findPath(const dtNavMeshQuery& navMeshQuery, const dtPolyRef startRef,
        const dtPolyRef endRef, const osg::Vec3f& startPos, const osg::Vec3f& endPos, const dtQueryFilter& queryFilter,
        std::span<dtPolyRef> pathBuffer)
    {
        int pathLen = 0;
        const auto status = navMeshQuery.findPath(startRef, endRef, startPos.ptr(), endPos.ptr(), &queryFilter,
            pathBuffer.data(), &pathLen, static_cast<int>(pathBuffer.size()));
        if (!dtStatusSucceed(status))
            return {};
        assert(pathLen >= 0);
        assert(static_cast<std::size_t>(pathLen) <= pathBuffer.size());
        return static_cast<std::size_t>(pathLen);
    }

    template <class OutputIterator>
    Status makeSmoothPath(const dtNavMeshQuery& navMeshQuery, const osg::Vec3f& start, const osg::Vec3f& end,
        std::span<dtPolyRef> polygonPath, std::size_t polygonPathSize, std::size_t maxSmoothPathSize,
        OutputIterator& out)
    {
        assert(polygonPathSize <= polygonPath.size());

        std::vector<float> cornerVertsBuffer(maxSmoothPathSize * 3);
        std::vector<unsigned char> cornerFlagsBuffer(maxSmoothPathSize);
        std::vector<dtPolyRef> cornerPolysBuffer(maxSmoothPathSize);
        int cornersCount = 0;
        constexpr int findStraightPathOptions = DT_STRAIGHTPATH_AREA_CROSSINGS | DT_STRAIGHTPATH_ALL_CROSSINGS;
        if (const dtStatus status = navMeshQuery.findStraightPath(start.ptr(), end.ptr(), polygonPath.data(),
                static_cast<int>(polygonPathSize), cornerVertsBuffer.data(), cornerFlagsBuffer.data(),
                cornerPolysBuffer.data(), &cornersCount, static_cast<int>(maxSmoothPathSize), findStraightPathOptions);
            dtStatusFailed(status))
            return Status::FindStraightPathFailed;

        for (int i = 0; i < cornersCount; ++i)
            *out++ = Misc::Convert::makeOsgVec3f(&cornerVertsBuffer[static_cast<std::size_t>(i) * 3]);

        return Status::Success;
    }

    template <class OutputIterator>
    Status findSmoothPath(const dtNavMeshQuery& navMeshQuery, const osg::Vec3f& halfExtents, const osg::Vec3f& start,
        const osg::Vec3f& end, const Flags includeFlags, const AreaCosts& areaCosts, const DetourSettings& settings,
        float endTolerance, OutputIterator out)
    {
        dtQueryFilter queryFilter;
        queryFilter.setIncludeFlags(includeFlags);
        queryFilter.setAreaCost(AreaType_water, areaCosts.mWater);
        queryFilter.setAreaCost(AreaType_door, areaCosts.mDoor);
        queryFilter.setAreaCost(AreaType_pathgrid, areaCosts.mPathgrid);
        queryFilter.setAreaCost(AreaType_ground, areaCosts.mGround);

        constexpr float polyDistanceFactor = 4;
        const osg::Vec3f polyHalfExtents = halfExtents * polyDistanceFactor;

        osg::Vec3f startNavMeshPos;
        dtPolyRef startRef = 0;
        if (const dtStatus status = navMeshQuery.findNearestPoly(
                start.ptr(), polyHalfExtents.ptr(), &queryFilter, &startRef, startNavMeshPos.ptr());
            dtStatusFailed(status) || startRef == 0)
            return Status::StartPolygonNotFound;

        osg::Vec3f endNavMeshPos;
        const osg::Vec3f endPolyHalfExtents = polyHalfExtents + osg::Vec3f(endTolerance, endTolerance, endTolerance);
        dtPolyRef endRef;
        if (const dtStatus status = navMeshQuery.findNearestPoly(
                end.ptr(), endPolyHalfExtents.ptr(), &queryFilter, &endRef, endNavMeshPos.ptr());
            dtStatusFailed(status) || endRef == 0)
            return Status::EndPolygonNotFound;

        std::vector<dtPolyRef> polygonPath(settings.mMaxPolygonPathSize);
        const auto polygonPathSize
            = findPath(navMeshQuery, startRef, endRef, startNavMeshPos, endNavMeshPos, queryFilter, polygonPath);

        if (!polygonPathSize.has_value())
            return Status::FindPathOverPolygonsFailed;

        if (*polygonPathSize == 0)
            return Status::Success;

        osg::Vec3f targetNavMeshPos;
        if (const dtStatus status = navMeshQuery.closestPointOnPoly(
                polygonPath[*polygonPathSize - 1], end.ptr(), targetNavMeshPos.ptr(), nullptr);
            dtStatusFailed(status))
            return Status::TargetPolygonNotFound;

        const bool partialPath = polygonPath[*polygonPathSize - 1] != endRef;
        const Status smoothStatus = makeSmoothPath(navMeshQuery, startNavMeshPos, targetNavMeshPos, polygonPath,
            *polygonPathSize, settings.mMaxSmoothPathSize, out);

        if (smoothStatus != Status::Success)
            return smoothStatus;

        return partialPath ? Status::PartialPath : Status::Success;
    }
}

#endif
