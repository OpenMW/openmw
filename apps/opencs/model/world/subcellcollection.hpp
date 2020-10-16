#ifndef CSM_WOLRD_SUBCOLLECTION_H
#define CSM_WOLRD_SUBCOLLECTION_H

#include "nestedidcollection.hpp"

namespace ESM
{
    class ESMReader;
}

namespace CSMWorld
{
    struct Cell;
    template<typename T, typename AT>
    class IdCollection;

    /// \brief Single type collection of top level records that are associated with cells
    template<typename ESXRecordT, typename IdAccessorT = IdAccessor<ESXRecordT> >
    class SubCellCollection : public NestedIdCollection<ESXRecordT, IdAccessorT>
    {
            const IdCollection<Cell>& mCells;

            void loadRecord (ESXRecordT& record, ESM::ESMReader& reader, bool& isDeleted) override;

        public:

            SubCellCollection (const IdCollection<Cell>& cells);
    };

    template<typename ESXRecordT, typename IdAccessorT>
    void SubCellCollection<ESXRecordT, IdAccessorT>::loadRecord (ESXRecordT& record,
                                                                 ESM::ESMReader& reader,
                                                                 bool& isDeleted)
    {
        record.load (reader, isDeleted, mCells);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    SubCellCollection<ESXRecordT, IdAccessorT>::SubCellCollection (
        const IdCollection<Cell>& cells)
    : mCells (cells)
    {}
}

#endif
