#ifndef OPENMW_ESM_CELLSTATE_H
#define OPENMW_ESM_CELLSTATE_H

#include "cellid.hpp"

#include "defs.hpp"

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // format 0, saved games only

    /// \note Does not include references
    struct CellState
    {
        CellId mId;

        float mWaterLevel;

        int mHasFogOfWar; // Do we have fog of war state (0 or 1)? (see fogstate.hpp)

        ESM::TimeStamp mLastRespawn;

        void load (ESMReader &esm);
        void save (ESMWriter &esm) const;
    };
}

#endif
