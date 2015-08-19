#include "cellstate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

void ESM::CellState::load (ESMReader &esm)
{
    mWaterLevel = 0;
    esm.getHNOT (mWaterLevel, "WLVL");

    mHasFogOfWar = false;
    esm.getHNOT (mHasFogOfWar, "HFOW");

    mLastRespawn.mDay = 0;
    mLastRespawn.mHour = 0;
    esm.getHNOT (mLastRespawn, "RESP");
}

void ESM::CellState::save (ESMWriter &esm) const
{
    if (!mId.mPaged)
        esm.writeHNT ("WLVL", mWaterLevel);

    esm.writeHNT ("HFOW", mHasFogOfWar);

    esm.writeHNT ("RESP", mLastRespawn);
}
