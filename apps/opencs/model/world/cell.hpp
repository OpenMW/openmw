#ifndef CSM_WOLRD_CELL_H
#define CSM_WOLRD_CELL_H

#include <vector>
#include <string>

#include <components/esm/loadcell.hpp>

namespace CSMWorld
{
    /// \brief Wrapper for Cell record
    ///
    /// \attention The mData.mX and mData.mY fields of the ESM::Cell struct are not used.
    /// Exterior cell coordinates are encoded in the cell ID.
    struct Cell : public ESM::Cell
    {
        std::string mId;

        void load (ESM::ESMReader &esm, bool &isDeleted);

    };
}

#endif
