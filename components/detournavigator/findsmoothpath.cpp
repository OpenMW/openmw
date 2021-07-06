#include "findsmoothpath.hpp"

#include <components/misc/convert.hpp>

#include <algorithm>
#include <array>

namespace DetourNavigator
{
    std::vector<dtPolyRef> fixupCorridor(const std::vector<dtPolyRef>& path, const std::vector<dtPolyRef>& visited)
    {
        std::vector<dtPolyRef>::const_reverse_iterator furthestVisited;

        // Find furthest common polygon.
        const auto it = std::find_if(path.rbegin(), path.rend(), [&] (dtPolyRef pathValue)
        {
            const auto it = std::find(visited.rbegin(), visited.rend(), pathValue);
            if (it == visited.rend())
                return false;
            furthestVisited = it;
            return true;
        });

        // If no intersection found just return current path.
        if (it == path.rend())
            return path;
        const auto furthestPath = it.base() - 1;

        // Concatenate paths.

        // visited: a_1 ... a_n x b_1 ... b_n
        //      furthestVisited ^
        //    path: C x D
        //            ^ furthestPath
        //  result: x b_n ... b_1 D

        std::vector<dtPolyRef> result;
        result.reserve(static_cast<std::size_t>(furthestVisited - visited.rbegin())
                       + static_cast<std::size_t>(path.end() - furthestPath) - 1);
        std::copy(visited.rbegin(), furthestVisited + 1, std::back_inserter(result));
        std::copy(furthestPath + 1, path.end(), std::back_inserter(result));

        return result;
    }

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
    std::vector<dtPolyRef> fixupShortcuts(const std::vector<dtPolyRef>& path, const dtNavMeshQuery& navQuery)
    {
        if (path.size() < 3)
            return path;

        // Get connected polygons
        const dtMeshTile* tile = nullptr;
        const dtPoly* poly = nullptr;
        if (dtStatusFailed(navQuery.getAttachedNavMesh()->getTileAndPolyByRef(path[0], &tile, &poly)))
            return path;

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
        for (std::size_t i = std::min(maxLookAhead, path.size()) - 1; i > 1 && cut == 0; i--)
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
            return path;

        std::vector<dtPolyRef> result;
        const auto offset = cut - 1;
        result.reserve(1 + path.size() - offset);
        result.push_back(path.front());
        std::copy(path.begin() + std::ptrdiff_t(offset), path.end(), std::back_inserter(result));
        return result;
    }

    std::optional<SteerTarget> getSteerTarget(const dtNavMeshQuery& navMeshQuery, const osg::Vec3f& startPos,
            const osg::Vec3f& endPos, const float minTargetDist, const std::vector<dtPolyRef>& path)
    {
        // Find steer target.
        SteerTarget result;
        constexpr int maxSteerPoints = 3;
        std::array<float, maxSteerPoints * 3> steerPath;
        std::array<unsigned char, maxSteerPoints> steerPathFlags;
        std::array<dtPolyRef, maxSteerPoints> steerPathPolys;
        int nsteerPath = 0;
        const dtStatus status = navMeshQuery.findStraightPath(startPos.ptr(), endPos.ptr(), path.data(),
            static_cast<int>(path.size()), steerPath.data(), steerPathFlags.data(), steerPathPolys.data(),
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

        dtVcopy(result.steerPos.ptr(), &steerPath[ns * 3]);
        result.steerPos.y() = startPos[1];
        result.steerPosFlag = steerPathFlags[ns];
        result.steerPosRef = steerPathPolys[ns];

        return result;
    }

    dtPolyRef findNearestPolyExpanding(const dtNavMeshQuery& query, const dtQueryFilter& filter,
            const osg::Vec3f& center, const osg::Vec3f& halfExtents)
    {
        dtPolyRef ref = 0;
        for (int i = 0; i < 3; ++i)
        {
            const dtStatus status = query.findNearestPoly(center.ptr(), (halfExtents * (1 << i)).ptr(), &filter, &ref, nullptr);
            if (!dtStatusFailed(status) && ref != 0)
                break;
        }
        return ref;
    }
}
