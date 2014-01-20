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
            // MSVC needs the constructor for a class inheriting a template to be defined in header
            RefCollection (Collection<Cell>& cells)
              : mCells (cells), mNextId (0)
            {}

            void load (ESM::ESMReader& reader, int cellIndex, bool base);
            ///< Load a sequence of references.

            std::string getNewId();
    };
}

#endif
