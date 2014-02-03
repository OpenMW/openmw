
#include "cellstate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

void ESM::CellState::load (ESMReader &esm)
{
    mWaterLevel = 0;
    esm.getHNOT (mWaterLevel, "WLVL");
}

void ESM::CellState::save (ESMWriter &esm) const
{
    if (!mId.mPaged)
        esm.writeHNT ("WLVL", mWaterLevel);
}