#ifndef OPENMW_MWWORLD_CELLUTILS_H
#define OPENMW_MWWORLD_CELLUTILS_H

#include <components/misc/constants.hpp>
#include <components/esm3/cellid.hpp>

#include <cmath>

namespace MWWorld
{
    inline ESM::CellId::CellIndex positionToIndex(float x, float y)
    {
        return {
            static_cast<int>(std::floor(x / Constants::CellSizeInUnits)),
            static_cast<int>(std::floor(y / Constants::CellSizeInUnits))
        };
    }
}

#endif
