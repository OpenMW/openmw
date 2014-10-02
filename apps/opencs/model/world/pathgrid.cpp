
#include "pathgrid.hpp"

#include <sstream>

void CSMWorld::Pathgrid::load (ESM::ESMReader &esm)
{
    ESM::Pathgrid::load (esm);

    if (mCell.empty())
    {
        std::ostringstream stream;

        stream << "#" << mData.mX << " " << mData.mY;

        mId = stream.str();
    }
    else
        mId = mCell;
}
