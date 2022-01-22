#ifndef CSM_WOLRD_REF_H
#define CSM_WOLRD_REF_H

#include <utility>

#include <components/esm3/cellref.hpp>

namespace CSMWorld
{
    /// \brief Wrapper for CellRef sub record
    struct CellRef : public ESM::CellRef
    {
        std::string mId;
        std::string mCell;
        std::string mOriginalCell;
        bool mNew; // new reference, not counted yet, ref num not assigned yet
        unsigned int mIdNum;

        CellRef();

        /// Calculate cell index based on coordinates (x and y)
        std::pair<int, int> getCellIndex() const;
    };
}

#endif
