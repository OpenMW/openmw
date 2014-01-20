
#include "cell.hpp"

#include <sstream>

void CSMWorld::Cell::load (ESM::ESMReader &esm)
{
    mName = mId;

    ESM::Cell::load (esm, false);

    if (!(mData.mFlags & Interior))
    {
        std::ostringstream stream;

        stream << "#" << mData.mX << " " << mData.mY;

        mId = stream.str();
    }
}

void CSMWorld::Cell::addRef (const std::string& id)
{
    mRefs.push_back (std::make_pair (id, false));
}