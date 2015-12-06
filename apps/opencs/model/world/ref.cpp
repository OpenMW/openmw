#include "ref.hpp"

#include <cmath>

CSMWorld::CellRef::CellRef()
{
    mId.clear();
    mCell.clear();
    mOriginalCell.clear();

    mRefNum.mIndex = 0;
    mRefNum.mContentFile = 0;
}

CSMWorld::CellRef::CellRef (CSMWorld::CellRef&& other) : ESM::CellRef (other)
{
    *this = std::move(other);
}

CSMWorld::CellRef& CSMWorld::CellRef::operator= (CSMWorld::CellRef&& other)
{
    if (this != &other)
    {
        ESM::CellRef::operator= (other);
        mId = std::move(other.mId);
        mCell = std::move(other.mCell);
        mOriginalCell = std::move(other.mOriginalCell);
    }

    return *this;
}

std::pair<int, int> CSMWorld::CellRef::getCellIndex() const
{
    const int cellSize = 8192;

    return std::make_pair (
        std::floor (mPos.pos[0]/cellSize), std::floor (mPos.pos[1]/cellSize));
}
