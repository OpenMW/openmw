#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_HEIGHFIELDSHAPE_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_HEIGHFIELDSHAPE_H

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
}

#endif
