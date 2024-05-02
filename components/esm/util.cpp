#include "util.hpp"

#include <components/esm3/loadland.hpp>
#include <components/esm4/loadland.hpp>

osg::Vec2f ESM::indexToPosition(const ESM::ExteriorCellLocation& cellIndex, bool centre)
{
    const int cellSize = ESM::getCellSize(cellIndex.mWorldspace);

    float x = static_cast<float>(cellSize * cellIndex.mX);
    float y = static_cast<float>(cellSize * cellIndex.mY);

    if (centre)
    {
        x += cellSize / 2;
        y += cellSize / 2;
    }

    return osg::Vec2f(x, y);
}

int ESM::getLandSize(ESM::RefId worldspaceId)
{
    return isEsm4Ext(worldspaceId) ? ESM4::Land::sVertsPerSide : ESM::Land::LAND_SIZE;
}
