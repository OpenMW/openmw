#include "cell.hpp"

#include <sstream>

void CSMWorld::Cell::load (ESM::ESMReader &esm, bool &isDeleted)
{
    ESM::Cell::load (esm, isDeleted, false);

    mId = mName;
    if (isExterior())
    {
        std::ostringstream stream;
        stream << "#" << mData.mX << " " << mData.mY;
        mId = stream.str();
    }
}
