#include "ref.hpp"

#include <components/esm/defs.hpp>

#include "cellcoordinates.hpp"

CSMWorld::CellRef::CellRef()
    : mNew(true)
    , mIdNum(0)
{
}

std::pair<int, int> CSMWorld::CellRef::getCellIndex() const
{
    return CellCoordinates::coordinatesToCellIndex(mPos.pos[0], mPos.pos[1]);
}
