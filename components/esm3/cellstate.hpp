#ifndef OPENMW_ESM_CELLSTATE_H
#define OPENMW_ESM_CELLSTATE_H

#include "components/esm/defs.hpp"
#include "components/esm/refid.hpp"
#include "timestamp.hpp"

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // format 0, saved games only

    /// \note Does not include references
    struct CellState
    {
        RefId mId;
        bool mIsInterior;
        float mWaterLevel;

        int32_t mHasFogOfWar; // Do we have fog of war state (0 or 1)? (see fogstate.hpp)

        TimeStamp mLastRespawn;

        void load(ESMReader& esm);
        void save(ESMWriter& esm) const;
    };
}

#endif
