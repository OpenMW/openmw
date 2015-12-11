#ifndef CSM_WOLRD_REF_H
#define CSM_WOLRD_REF_H

#include <utility>

#include <components/esm/cellref.hpp>

namespace CSMWorld
{
    /// \brief Wrapper for CellRef sub record
    struct CellRef : public ESM::CellRef
    {
        unsigned int mIdNum;

        std::string mId;
        std::string mCell;
        std::string mOriginalCell;

        CellRef();
        CellRef(const CellRef&) = default;
        CellRef& operator= (const CellRef&) = default;

        CellRef (CellRef&& other);
        CellRef& operator= (CellRef&& other);

        /// Calculate cell index based on coordinates (x and y)
        std::pair<int, int> getCellIndex() const;
    };
}

#endif
