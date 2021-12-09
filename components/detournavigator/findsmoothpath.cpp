#include "findsmoothpath.hpp"

#include <components/misc/convert.hpp>

#include <algorithm>
#include <array>

namespace DetourNavigator
{
    std::size_t fixupCorridor(std::vector<dtPolyRef>& path, std::size_t pathSize, const std::vector<dtPolyRef>& visited)
    {
        std::vector<dtPolyRef>::const_reverse_iterator furthestVisited;

        // Find furthest common polygon.
        const auto begin = path.begin();
        const auto end = path.begin() + pathSize;
        const std::reverse_iterator rbegin(end);
        const std::reverse_iterator rend(begin);
        const auto it = std::find_if(rbegin, rend, [&] (dtPolyRef pathValue)
        {
            const auto it = std::find(visited.rbegin(), visited.rend(), pathValue);
            if (it == visited.rend())
                return false;
            furthestVisited = it;
            return true;
        });

        // If no intersection found just return current path.
        if (it == rend)
            return pathSize;
        const auto furthestPath = it.base() - 1;

        // Concatenate paths.

        // visited: a_1 ... a_n x b_1 ... b_n
        //      furthestVisited ^
        //    path: C x D            E
        //            ^ furthestPath ^ path.size() - (furthestVisited + 1 - visited.rbegin())
        //  result: x b_n ... b_1 D

        const std::size_t required = static_cast<std::size_t>(furthestVisited + 1 - visited.rbegin());
        const auto newEnd = std::copy(furthestPath + 1, std::min(begin + path.size(), end), begin + required);
        std::copy(visited.rbegin(), furthestVisited + 1, begin);

        return static_cast<std::size_t>(newEnd - begin);
    }

    std::size_t fixupShortcuts(dtPolyRef* path, std::size_t pathSize, const dtNavMeshQuery& navQuery)
    {
        if (pathSize < 3)
            return pathSize;

        // Get connected polygons
        const dtMeshTile* tile = nullptr;
        const dtPoly* poly = nullptr;
        if (dtStatusFailed(navQuery.getAttachedNavMesh()->getTileAndPolyByRef(path[0], &tile, &poly)))
            return pathSize;

        const std::size_t maxNeis = 16;
        std::array<dtPolyRef, maxNeis> neis;
        std::size_t nneis = 0;

        for (unsigned int k = poly->firstLink; k != DT_NULL_LINK; k = tile->links[k].next)
        {
            const dtLink* link = &tile->links[k];
            if (link->ref != 0)
            {
                if (nneis < maxNeis)
                    neis[nneis++] = link->ref;
            }
        }

        // If any of the neighbour polygons is within the next few polygons
        // in the path, short cut to that polygon directly.
        const std::size_t maxLookAhead = 6;
        std::size_t cut = 0;
        for (std::size_t i = std::min(maxLookAhead, pathSize) - 1; i > 1 && cut == 0; i--)
        {
            for (std::size_t j = 0; j < nneis; j++)
            {
                if (path[i] == neis[j])
                {
                    cut = i;
                    break;
                }
            }
        }
        if (cut <= 1)
            return pathSize;

        const std::ptrdiff_t offset = static_cast<std::ptrdiff_t>(cut) - 1;
        std::copy(path + offset, path + pathSize, path);
        return pathSize - offset;
    }

    std::optional<SteerTarget> getSteerTarget(const dtNavMeshQuery& navMeshQuery, const osg::Vec3f& startPos,
            const osg::Vec3f& endPos, const float minTargetDist, const dtPolyRef* path, const std::size_t pathSize)
    {
        // Find steer target.
        SteerTarget result;
        constexpr int maxSteerPoints = 3;
        std::array<float, maxSteerPoints * 3> steerPath;
        std::array<unsigned char, maxSteerPoints> steerPathFlags;
        std::array<dtPolyRef, maxSteerPoints> steerPathPolys;
        int nsteerPath = 0;
        const dtStatus status = navMeshQuery.findStraightPath(startPos.ptr(), endPos.ptr(), path,
            static_cast<int>(pathSize), steerPath.data(), steerPathFlags.data(), steerPathPolys.data(),
            &nsteerPath, maxSteerPoints);
        if (dtStatusFailed(status))
            return std::nullopt;
        assert(nsteerPath >= 0);
        if (!nsteerPath)
            return std::nullopt;

        // Find vertex far enough to steer to.
        std::size_t ns = 0;
        while (ns < static_cast<std::size_t>(nsteerPath))
        {
            // Stop at Off-Mesh link or when point is further than slop away.
            if ((steerPathFlags[ns] & DT_STRAIGHTPATH_OFFMESH_CONNECTION) ||
                    !inRange(Misc::Convert::makeOsgVec3f(&steerPath[ns * 3]), startPos, minTargetDist))
                break;
            ns++;
        }
        // Failed to find good point to steer to.
        if (ns >= static_cast<std::size_t>(nsteerPath))
            return std::nullopt;

        dtVcopy(result.mSteerPos.ptr(), &steerPath[ns * 3]);
        result.mSteerPos.y() = startPos[1];
        result.mSteerPosFlag = steerPathFlags[ns];
        result.mSteerPosRef = steerPathPolys[ns];

        return result;
    }

    dtPolyRef findNearestPoly(const dtNavMeshQuery& query, const dtQueryFilter& filter,
            const osg::Vec3f& center, const osg::Vec3f& halfExtents)
    {
        dtPolyRef ref = 0;
        const dtStatus status = query.findNearestPoly(center.ptr(), halfExtents.ptr(), &filter, &ref, nullptr);
        if (!dtStatusSucceed(status))
            return 0;
        return ref;
    }
}
