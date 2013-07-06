
#include "refcollection.hpp"

#include <sstream>

#include "ref.hpp"
#include "cell.hpp"

CSMWorld::RefCollection::RefCollection (Collection<Cell>& cells)
: mCells (cells), mNextId (0)
{}

void CSMWorld::RefCollection::load (ESM::ESMReader& reader, int cellIndex, bool base)
{
    Record<Cell> cell = mCells.getRecord (cellIndex);

    Cell& cell2 = base ? cell.mBase : cell.mModified;

    cell2.restore (reader, 0); /// \todo fix the index

    CellRef ref;

    while (cell2.getNextRef (reader, ref))
    {
        /// \todo handle deleted and moved references
        std::ostringstream stream;
        stream << "ref#" << mNextId++;

        ref.load (reader, cell2, stream.str());

        Record<CellRef> record2;
        record2.mState = base ? RecordBase::State_BaseOnly : RecordBase::State_ModifiedOnly;
        (base ? record2.mBase : record2.mModified) = ref;

        appendRecord (record2);
    }

    mCells.setRecord (cellIndex, cell);
}