#ifndef OPENMW_MWWORLD_CELLUTILS_H
#define OPENMW_MWWORLD_CELLUTILS_H

#include <components/misc/constants.hpp>

#include <osg/Vec2i>

#include <cmath>

namespace MWWorld
{
    inline osg::Vec2i positionToCellIndex(float x, float y, bool esm4Ext = false)
    {
        const float cellSize = esm4Ext ? Constants::ESM4CellSizeInUnits : Constants::CellSizeInUnits;
        return { static_cast<int>(std::floor(x / cellSize)), static_cast<int>(std::floor(y / cellSize)) };
    }
}

#endif
