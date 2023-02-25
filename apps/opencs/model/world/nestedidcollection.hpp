#ifndef CSM_WOLRD_NESTEDIDCOLLECTION_H
#define CSM_WOLRD_NESTEDIDCOLLECTION_H

#include <map>
#include <memory>
#include <stdexcept>

#include "collection.hpp"
#include "nestedcollection.hpp"
#include "nestedcolumnadapter.hpp"

namespace ESM
{
    class ESMReader;
}

namespace CSMWorld
{
    struct NestedTableWrapperBase;
    struct Cell;
    struct ColumnBase;

    template <typename ESXRecordT>
    class IdCollection;

    template <typename ESXRecordT>
    class NestedColumnAdapter;

    template <typename ESXRecordT>
    class NestedIdCollection : public IdCollection<ESXRecordT>, public NestedCollection
    {
        std::map<const ColumnBase*, NestedColumnAdapter<ESXRecordT>*> mAdapters;

        const NestedColumnAdapter<ESXRecordT>& getAdapter(const ColumnBase& column) const;

    public:
        NestedIdCollection();
        ~NestedIdCollection() override;

        void addNestedRow(int row, int column, int position) override;

        void removeNestedRows(int row, int column, int subRow) override;

        QVariant getNestedData(int row, int column, int subRow, int subColumn) const override;

        void setNestedData(int row, int column, const QVariant& data, int subRow, int subColumn) override;

        NestedTableWrapperBase* nestedTable(int row, int column) const override;

        void setNestedTable(int row, int column, const NestedTableWrapperBase& nestedTable) override;

        int getNestedRowsCount(int row, int column) const override;

        int getNestedColumnsCount(int row, int column) const override;

        // this method is inherited from NestedCollection, not from Collection<ESXRecordT>
        NestableColumn* getNestableColumn(int column) override;

        void addAdapter(std::pair<const ColumnBase*, NestedColumnAdapter<ESXRecordT>*> adapter);
    };

    template <typename ESXRecordT>
    NestedIdCollection<ESXRecordT>::NestedIdCollection()
    {
    }

    template <typename ESXRecordT>
    NestedIdCollection<ESXRecordT>::~NestedIdCollection()
    {
        for (typename std::map<const ColumnBase*, NestedColumnAdapter<ESXRecordT>*>::iterator iter(mAdapters.begin());
             iter != mAdapters.end(); ++iter)
        {
            delete (*iter).second;
        }
    }

    template <typename ESXRecordT>
    void NestedIdCollection<ESXRecordT>::addAdapter(
        std::pair<const ColumnBase*, NestedColumnAdapter<ESXRecordT>*> adapter)
    {
        mAdapters.insert(adapter);
    }

    template <typename ESXRecordT>
    const NestedColumnAdapter<ESXRecordT>& NestedIdCollection<ESXRecordT>::getAdapter(const ColumnBase& column) const
    {
        typename std::map<const ColumnBase*, NestedColumnAdapter<ESXRecordT>*>::const_iterator iter
            = mAdapters.find(&column);

        if (iter == mAdapters.end())
            throw std::logic_error("No such column in the nestedidadapter");

        return *iter->second;
    }

    template <typename ESXRecordT>
    void NestedIdCollection<ESXRecordT>::addNestedRow(int row, int column, int position)
    {
        auto record = std::make_unique<Record<ESXRecordT>>();
        record->assign(Collection<ESXRecordT>::getRecord(row));

        getAdapter(Collection<ESXRecordT>::getColumn(column)).addRow(*record, position);

        Collection<ESXRecordT>::setRecord(row, std::move(record));
    }

    template <typename ESXRecordT>
    void NestedIdCollection<ESXRecordT>::removeNestedRows(int row, int column, int subRow)
    {
        auto record = std::make_unique<Record<ESXRecordT>>();
        record->assign(Collection<ESXRecordT>::getRecord(row));

        getAdapter(Collection<ESXRecordT>::getColumn(column)).removeRow(*record, subRow);

        Collection<ESXRecordT>::setRecord(row, std::move(record));
    }

    template <typename ESXRecordT>
    QVariant NestedIdCollection<ESXRecordT>::getNestedData(int row, int column, int subRow, int subColumn) const
    {
        return getAdapter(Collection<ESXRecordT>::getColumn(column))
            .getData(Collection<ESXRecordT>::getRecord(row), subRow, subColumn);
    }

    template <typename ESXRecordT>
    void NestedIdCollection<ESXRecordT>::setNestedData(
        int row, int column, const QVariant& data, int subRow, int subColumn)
    {
        auto record = std::make_unique<Record<ESXRecordT>>();
        record->assign(Collection<ESXRecordT>::getRecord(row));

        getAdapter(Collection<ESXRecordT>::getColumn(column)).setData(*record, data, subRow, subColumn);

        Collection<ESXRecordT>::setRecord(row, std::move(record));
    }

    template <typename ESXRecordT>
    CSMWorld::NestedTableWrapperBase* NestedIdCollection<ESXRecordT>::nestedTable(int row, int column) const
    {
        return getAdapter(Collection<ESXRecordT>::getColumn(column)).table(Collection<ESXRecordT>::getRecord(row));
    }

    template <typename ESXRecordT>
    void NestedIdCollection<ESXRecordT>::setNestedTable(
        int row, int column, const CSMWorld::NestedTableWrapperBase& nestedTable)
    {
        auto record = std::make_unique<Record<ESXRecordT>>();
        record->assign(Collection<ESXRecordT>::getRecord(row));

        getAdapter(Collection<ESXRecordT>::getColumn(column)).setTable(*record, nestedTable);

        Collection<ESXRecordT>::setRecord(row, std::move(record));
    }

    template <typename ESXRecordT>
    int NestedIdCollection<ESXRecordT>::getNestedRowsCount(int row, int column) const
    {
        return getAdapter(Collection<ESXRecordT>::getColumn(column))
            .getRowsCount(Collection<ESXRecordT>::getRecord(row));
    }

    template <typename ESXRecordT>
    int NestedIdCollection<ESXRecordT>::getNestedColumnsCount(int row, int column) const
    {
        const ColumnBase& nestedColumn = Collection<ESXRecordT>::getColumn(column);
        int numRecords = Collection<ESXRecordT>::getSize();
        if (row >= 0 && row < numRecords)
        {
            const Record<ESXRecordT>& record = Collection<ESXRecordT>::getRecord(row);
            return getAdapter(nestedColumn).getColumnsCount(record);
        }
        else
        {
            // If the row is invalid (or there no records), retrieve the column count using a blank record
            const Record<ESXRecordT> record;
            return getAdapter(nestedColumn).getColumnsCount(record);
        }
    }

    template <typename ESXRecordT>
    CSMWorld::NestableColumn* NestedIdCollection<ESXRecordT>::getNestableColumn(int column)
    {
        return Collection<ESXRecordT>::getNestableColumn(column);
    }
}

#endif // CSM_WOLRD_NESTEDIDCOLLECTION_H
