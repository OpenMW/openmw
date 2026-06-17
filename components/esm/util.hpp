#ifndef OPENMW_COMPONENTS_ESM_UTIL_H
#define OPENMW_COMPONENTS_ESM_UTIL_H

#include <cmath>

#include <osg/Vec2f>

#include "components/esm3/loadcell.hpp"
#include "components/misc/constants.hpp"

#include "exteriorcelllocation.hpp"
#include "refid.hpp"

namespace ESM
{
    inline bool isEsm4Ext(ESM::RefId worldspaceId)
    {
        return worldspaceId != ESM::Cell::sDefaultWorldspaceId;
    }

    inline int getCellSize(ESM::RefId worldspaceId)
    {
        return isEsm4Ext(worldspaceId) ? Constants::ESM4CellSizeInUnits : Constants::CellSizeInUnits;
    }

    // Vertex count of a side of a land record
    int getLandSize(ESM::RefId worldspaceId);

    inline ESM::ExteriorCellLocation positionToExteriorCellLocation(
        float x, float y, ESM::RefId worldspaceId = ESM::Cell::sDefaultWorldspaceId)
    {
        const float cellSize = static_cast<float>(getCellSize(worldspaceId));
        return { static_cast<int>(std::floor(x / cellSize)), static_cast<int>(std::floor(y / cellSize)), worldspaceId };
    }

    // Convert exterior cell location to position.
    osg::Vec2f indexToPosition(const ESM::ExteriorCellLocation& cellIndex, bool centre = false);
}

#endif
