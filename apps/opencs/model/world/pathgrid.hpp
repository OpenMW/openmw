#ifndef CSM_WOLRD_PATHGRID_H
#define CSM_WOLRD_PATHGRID_H

#include <string>

#include <components/esm/refid.hpp>
#include <components/esm3/loadpgrd.hpp>

namespace ESM
{
    class ESMReader;
}

namespace CSMWorld
{
    struct Cell;
    template <typename T>
    class IdCollection;

    /// \brief Wrapper for Pathgrid record
    ///
    /// \attention The mData.mX and mData.mY fields of the ESM::Pathgrid struct are not used.
    /// Exterior cell coordinates are encoded in the pathgrid ID.
    struct Pathgrid : public ESM::Pathgrid
    {
        ESM::RefId mId;

        void load(ESM::ESMReader& esm, bool& isDeleted, const IdCollection<Cell>& cells);
        void load(ESM::ESMReader& esm, bool& isDeleted);
    };
}

#endif
