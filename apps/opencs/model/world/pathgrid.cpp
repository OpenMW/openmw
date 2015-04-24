#include "cell.hpp"
#include "idcollection.hpp"
#include "pathgrid.hpp"

#include <sstream>

void CSMWorld::Pathgrid::load (ESM::ESMReader &esm, const IdCollection<Cell>& cells)
{
    load (esm);

    // correct ID
    if (!mId.empty() && mId[0]!='#' && cells.searchId (mId)==-1)
    {
        std::ostringstream stream;

        stream << "#" << mData.mX << " " << mData.mY;

        mId = stream.str();
    }
}

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
