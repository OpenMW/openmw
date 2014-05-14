#ifndef CSM_WOLRD_REF_H
#define CSM_WOLRD_REF_H

#include <components/esm/cellref.hpp>

namespace ESM
{
    class ESMReader;
}

namespace CSMWorld
{
    class Cell;

    /// \brief Wrapper for CellRef sub record
    struct CellRef : public ESM::CellRef
    {
        std::string mId;
        std::string mCell;

        void load (ESM::ESMReader &esm, Cell& cell, const std::string& id);
        ///< Load cell ref and register it with \a cell.
    };
}

#endif
