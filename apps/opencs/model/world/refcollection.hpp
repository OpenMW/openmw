#ifndef CSM_WOLRD_REFCOLLECTION_H
#define CSM_WOLRD_REFCOLLECTION_H

#include "collection.hpp"
#include "ref.hpp"
#include "record.hpp"

namespace CSMWorld
{
    struct Cell;

    /// \brief References in cells
    class RefCollection : public Collection<CellRef>
    {
            Collection<Cell>& mCells;
            int mNextId;

        public:

            RefCollection (Collection<Cell>& cells);

            void load (ESM::ESMReader& reader, int cellIndex, bool base);
            ///< Load a sequence of references.
    };
}

#endif
