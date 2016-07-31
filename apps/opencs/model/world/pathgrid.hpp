#ifndef CSM_WOLRD_PATHGRID_H
#define CSM_WOLRD_PATHGRID_H

#include <vector>
#include <string>

#include <components/esm/loadpgrd.hpp>

namespace CSMWorld
{
    struct Cell;
    template<typename T, typename AT>
    class IdCollection;

    /// \brief Wrapper for Pathgrid record
    ///
    /// \attention The mData.mX and mData.mY fields of the ESM::Pathgrid struct are not used.
    /// Exterior cell coordinates are encoded in the pathgrid ID.
    struct Pathgrid : public ESM::Pathgrid
    {
        std::string mId;

        void load (ESM::ESMReader &esm, bool &isDeleted, const IdCollection<Cell>& cells);
        void load (ESM::ESMReader &esm, bool &isDeleted);
    };
}

#endif
