
#include "ref.hpp"

#include "cell.hpp"

void CSMWorld::CellRef::load (ESM::ESMReader &esm)
{
    // The CellRef is not loaded here. Because of the unfortunate way how the ESMReader and the cell
    // record is constructed, we do not have enough context in this function to perform a load.

//    mId = id;

//    cell.getNextRef (esm, *this);

//    if (!mDeleted)
//        cell.addRef (mId);
}
