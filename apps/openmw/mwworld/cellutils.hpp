#ifndef OPENMW_MWWORLD_CELLUTILS_H
#define OPENMW_MWWORLD_CELLUTILS_H

#include <components/misc/constants.hpp>

#include <osg/Vec2i>

#include <cmath>

namespace MWWorld
{
    inline osg::Vec2i positionToCellIndex(float x, float y)
    {
        return {
            static_cast<int>(std::floor(x / Constants::CellSizeInUnits)),
            static_cast<int>(std::floor(y / Constants::CellSizeInUnits))
        };
    }
}

#endif
