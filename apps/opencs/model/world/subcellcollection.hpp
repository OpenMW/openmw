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

    /// \brief Single type collection of top level records that are associated with cells
    template <typename ESXRecordT>
    class SubCellCollection final : public NestedIdCollection<ESXRecordT>
    {
        const IdCollection<Cell>& mCells;

        void loadRecord(ESXRecordT& record, ESM::ESMReader& reader, bool& isDeleted, bool base) override;

    public:
        SubCellCollection(const IdCollection<Cell>& cells);
    };

    template <typename ESXRecordT>
    void SubCellCollection<ESXRecordT>::loadRecord(
        ESXRecordT& record, ESM::ESMReader& reader, bool& isDeleted, bool base)
    {
        record.load(reader, isDeleted, mCells);
    }

    template <typename ESXRecordT>
    SubCellCollection<ESXRecordT>::SubCellCollection(const IdCollection<Cell>& cells)
        : mCells(cells)
    {
    }
}

#endif
