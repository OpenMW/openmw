#include "ref.hpp"

#include <cmath>

CSMWorld::CellRef::CellRef()
{
    mRefNum.mIndex = 0;
    mRefNum.mContentFile = 0;
}

std::pair<int, int> CSMWorld::CellRef::getCellIndex() const
{
    const int cellSize = 8192;

    return std::make_pair (
        std::floor (mPos.pos[0]/cellSize), std::floor (mPos.pos[1]/cellSize));
}
