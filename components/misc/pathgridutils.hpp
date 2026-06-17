#ifndef OPENMW_COMPONENTS_MISC_PATHGRIDUTILS_H
#define OPENMW_COMPONENTS_MISC_PATHGRIDUTILS_H

#include "convert.hpp"

#include <components/esm3/loadpgrd.hpp>

#include <osg/Vec3f>

#include <stdexcept>

namespace Misc
{
    // Slightly cheaper version for comparisons.
    // Caller needs to be careful for very short distances (i.e. less than 1)
    // or when accumuating the results i.e. (a + b)^2 != a^2 + b^2
    //
    inline float distanceSquared(const ESM::Pathgrid::Point& point, const osg::Vec3f& pos)
    {
        return (Misc::Convert::makeOsgVec3f(point) - pos).length2();
    }

    // Return the closest pathgrid point index from the specified position
    // coordinates.  NOTE: Does not check if there is a sensible way to get there
    // (e.g. a cliff in front).
    //
    // NOTE: pos is expected to be in local coordinates, as is grid->mPoints
    //
    inline std::size_t getClosestPoint(const ESM::Pathgrid& grid, const osg::Vec3f& pos)
    {
        if (grid.mPoints.empty())
            throw std::invalid_argument("Pathgrid has no points");

        float minDistance = distanceSquared(grid.mPoints[0], pos);
        std::size_t closestIndex = 0;

        // TODO: if this full scan causes performance problems mapping pathgrid
        //       points to a quadtree may help
        for (std::size_t i = 1; i < grid.mPoints.size(); ++i)
        {
            const float distance = distanceSquared(grid.mPoints[i], pos);
            if (minDistance > distance)
            {
                minDistance = distance;
                closestIndex = i;
            }
        }

        return closestIndex;
    }
}

#endif
