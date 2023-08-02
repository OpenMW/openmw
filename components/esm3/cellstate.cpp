#include "cellstate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

    void CellState::load(ESMReader& esm)
    {
        mWaterLevel = 0;
        esm.getHNOT(mWaterLevel, "WLVL");

        mHasFogOfWar = false;
        esm.getHNOT(mHasFogOfWar, "HFOW");

        mLastRespawn.mDay = 0;
        mLastRespawn.mHour = 0;
        if (esm.peekNextSub("RESP"))
            mLastRespawn.load(esm, "RESP");
    }

    void CellState::save(ESMWriter& esm) const
    {
        if (mIsInterior)
            esm.writeHNT("WLVL", mWaterLevel);

        esm.writeHNT("HFOW", mHasFogOfWar);

        esm.writeHNT("RESP", mLastRespawn);
    }

}
