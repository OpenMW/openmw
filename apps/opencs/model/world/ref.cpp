
#include "ref.hpp"

#include "cell.hpp"

void CSMWorld::CellRef::load (ESM::ESMReader &esm, Cell& cell, const std::string& id)
{
    mId = id;

    cell.getNextRef (esm, *this);

    if (!mDeleted)
        cell.addRef (mId);
}