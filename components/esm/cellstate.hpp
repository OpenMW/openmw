#ifndef OPENMW_ESM_CELLSTATE_H
#define OPENMW_ESM_CELLSTATE_H

#include "cellid.hpp"

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

        void load (ESMReader &esm);
        void save (ESMWriter &esm) const;
    };
}

#endif