#ifndef CSM_WOLRD_CELL_H
#define CSM_WOLRD_CELL_H

#include <components/esm3/loadcell.hpp>

namespace ESM
{
    class ESMReader;
}

namespace CSMWorld
{
    /// \brief Wrapper for Cell record
    ///
    /// \attention The mData.mX and mData.mY fields of the ESM::Cell struct are not used.
    /// Exterior cell coordinates are encoded in the cell ID.
    struct Cell : public ESM::Cell
    {
        ESM::RefId mId;

        void load(ESM::ESMReader& esm, bool& isDeleted)
        {
            ESM::Cell::load(esm, isDeleted, false);

            mId = ESM::RefId::stringRefId(ESM::Cell::mId.toString());
        }
    };
}

#endif
