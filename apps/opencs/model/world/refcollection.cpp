
#include "refcollection.hpp"

#include <sstream>

#include "ref.hpp"
#include "cell.hpp"
#include "universalid.hpp"
#include "record.hpp"

void CSMWorld::RefCollection::load (ESM::ESMReader& reader, int cellIndex, bool base)
{
    Record<Cell> cell = mCells.getRecord (cellIndex);

    Cell& cell2 = base ? cell.mBase : cell.mModified;

    CellRef ref;

    bool deleted = false;
    while (cell2.getNextRef (reader, ref, deleted))
    {
        /// \todo handle deleted and moved references
        ref.load (reader, cell2, getNewId());

        Record<CellRef> record2;
        record2.mState = base ? RecordBase::State_BaseOnly : RecordBase::State_ModifiedOnly;
        (base ? record2.mBase : record2.mModified) = ref;

        appendRecord (record2);
    }

    mCells.setRecord (cellIndex, cell);
}

std::string CSMWorld::RefCollection::getNewId()
{
    std::ostringstream stream;
    stream << "ref#" << mNextId++;
    return stream.str();
}
