
#include "cellid.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

void ESM::CellId::load (ESMReader &esm)
{
    mWorldspace = esm.getHNString ("SPAC");

    if (esm.isNextSub ("CIDX"))
    {
        esm.getHT (mIndex, 8);
        mPaged = true;
    }
    else
        mPaged = false;
}

void ESM::CellId::save (ESMWriter &esm) const
{
    esm.writeHNString ("SPAC", mWorldspace);

    if (mPaged)
        esm.writeHNT ("CIDX", mIndex, 8);
}