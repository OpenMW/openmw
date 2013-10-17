#ifndef CSM_WOLRD_CELL_H
#define CSM_WOLRD_CELL_H

#include <vector>
#include <string>

#include <components/esm/loadcell.hpp>

namespace CSMWorld
{
    /// \brief Wrapper for Cell record
    struct Cell : public ESM::Cell
    {
        std::string mId;
        std::vector<std::pair<std::string, bool> > mRefs; // ID, modified
        std::vector<std::string> mDeletedRefs;

        void load (ESM::ESMReader &esm);

        void addRef (const std::string& id);
    };
}

#endif