#ifndef CSM_WOLRD_CELL_H
#define CSM_WOLRD_CELL_H

#include <components/esm/loadcell.hpp>

namespace CSMWorld
{
    /// \brief Wrapper for Cell record
    struct Cell : public ESM::Cell
    {
        std::string mId;

        void load (ESM::ESMReader &esm);
    };
}

#endif