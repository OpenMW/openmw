
#include "cell.hpp"

#include <sstream>

void CSMWorld::Cell::load (ESM::ESMReader &esm)
{
    ESM::Cell::load (esm, false);

    mId = mName;
    if (!(mData.mFlags & Interior))
    {
        std::ostringstream stream;
        stream << "#" << mData.mX << " " << mData.mY;
        mId = stream.str();
    }
}
