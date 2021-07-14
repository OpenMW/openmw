#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_TILEBOUNDS_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_TILEBOUNDS_H

#include <osg/Vec2f>

#include <algorithm>
#include <optional>
#include <tuple>

namespace DetourNavigator
{
    struct TileBounds
    {
        osg::Vec2f mMin;
        osg::Vec2f mMax;
    };

    inline bool operator<(const TileBounds& lhs, const TileBounds& rhs) noexcept
    {
        return std::tie(lhs.mMin, lhs.mMax) < std::tie(rhs.mMin, rhs.mMax);
    }

    inline std::optional<TileBounds> getIntersection(const TileBounds& a, const TileBounds& b) noexcept
    {
        const float minX = std::max(a.mMin.x(), b.mMin.x());
        const float maxX = std::min(a.mMax.x(), b.mMax.x());
        if (minX > maxX)
            return std::nullopt;
        const float minY = std::max(a.mMin.y(), b.mMin.y());
        const float maxY = std::min(a.mMax.y(), b.mMax.y());
        if (minY > maxY)
            return std::nullopt;
        return TileBounds {osg::Vec2f(minX, minY), osg::Vec2f(maxX, maxY)};
    }
}

#endif
