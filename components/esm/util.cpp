#include "util.hpp"

osg::Vec2 ESM::indexToPosition(const ESM::ExteriorCellIndex& cellIndex, bool centre)
{
    const int cellSize = ESM::getCellSize(cellIndex.mWorldspace);

    float x = static_cast<float>(cellSize * cellIndex.mX);
    float y = static_cast<float>(cellSize * cellIndex.mY);

    if (centre)
    {
        x += cellSize / 2;
        y += cellSize / 2;
    }
    return osg::Vec2(x, y);
}
