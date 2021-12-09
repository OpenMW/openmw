#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_HEIGHFIELDSHAPE_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_HEIGHFIELDSHAPE_H

#include <components/bullethelpers/heightfield.hpp>

#include <osg/Vec2i>

#include <cstddef>
#include <variant>

namespace DetourNavigator
{
    struct HeightfieldPlane
    {
        float mHeight;
    };

    struct HeightfieldSurface
    {
        const float* mHeights;
        std::size_t mSize;
        float mMinHeight;
        float mMaxHeight;
    };

    using HeightfieldShape = std::variant<HeightfieldPlane, HeightfieldSurface>;

    inline btVector3 getHeightfieldShift(const HeightfieldPlane& v, const osg::Vec2i& cellPosition, int cellSize)
    {
        return BulletHelpers::getHeightfieldShift(cellPosition.x(), cellPosition.y(), cellSize, v.mHeight, v.mHeight);
    }

    inline btVector3 getHeightfieldShift(const HeightfieldSurface& v, const osg::Vec2i& cellPosition, int cellSize)
    {
        return BulletHelpers::getHeightfieldShift(cellPosition.x(), cellPosition.y(), cellSize, v.mMinHeight, v.mMaxHeight);
    }

    inline btVector3 getHeightfieldShift(const HeightfieldShape& v, const osg::Vec2i& cellPosition, int cellSize)
    {
        return std::visit([&] (const auto& w) { return getHeightfieldShift(w, cellPosition, cellSize); }, v);
    }
}

#endif
