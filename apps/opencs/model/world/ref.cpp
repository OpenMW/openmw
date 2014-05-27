
#include "ref.hpp"

CSMWorld::CellRef::CellRef()
{
    mRefNum.mIndex = 0;

    // special marker: This reference does not have a RefNum assign to it yet.
    mRefNum.mContentFile = -2;
}