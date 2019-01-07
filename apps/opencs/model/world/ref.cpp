#include "ref.hpp"

#include "cellcoordinates.hpp"

CSMWorld::CellRef::CellRef() : mNew (true)
{
    mRefNum.mIndex = 0;
    mRefNum.mContentFile = 0;
}

std::pair<int, int> CSMWorld::CellRef::getCellIndex() const
{
    return CellCoordinates::coordinatesToCellIndex (mPos.pos[0], mPos.pos[1]);
}
