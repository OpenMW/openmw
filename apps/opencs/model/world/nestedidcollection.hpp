#ifndef CSM_WOLRD_NESTEDIDCOLLECTION_H
#define CSM_WOLRD_NESTEDIDCOLLECTION_H

#include <map>
#include <stdexcept>

#include "nestedcollection.hpp"
#include "nestedcoladapterimp.hpp"

namespace ESM
{
    class ESMReader;
}

namespace CSMWorld
{
    struct NestedTableWrapperBase;
    struct Cell;

    template<typename T, typename AT>
    class IdCollection;

    template<typename ESXRecordT, typename IdAccessorT = IdAccessor<ESXRecordT> >
    class NestedIdCollection : public IdCollection<ESXRecordT, IdAccessorT>, public NestedCollection
    {
            std::map<const ColumnBase*, NestedColumnAdapter<ESXRecordT>* > mAdapters;

            const NestedColumnAdapter<ESXRecordT>& getAdapter(const ColumnBase &column) const;

        public:

            NestedIdCollection ();
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
            NestableColumn *getNestableColumn(int column) override;

            void addAdapter(std::pair<const ColumnBase*, NestedColumnAdapter<ESXRecordT>* > adapter);
    };

    template<typename ESXRecordT, typename IdAccessorT>
    NestedIdCollection<ESXRecordT, IdAccessorT>::NestedIdCollection ()
    {}

    template<typename ESXRecordT, typename IdAccessorT>
    NestedIdCollection<ESXRecordT, IdAccessorT>::~NestedIdCollection()
    {
        for (typename std::map<const ColumnBase *, NestedColumnAdapter<ESXRecordT>* >::iterator
                iter (mAdapters.begin()); iter!=mAdapters.end(); ++iter)
        {
            delete (*iter).second;
        }
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void NestedIdCollection<ESXRecordT, IdAccessorT>::addAdapter(std::pair<const ColumnBase*,
            NestedColumnAdapter<ESXRecordT>* > adapter)
    {
        mAdapters.insert(adapter);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    const NestedColumnAdapter<ESXRecordT>& NestedIdCollection<ESXRecordT, IdAccessorT>::getAdapter(const ColumnBase &column) const
    {
        typename std::map<const ColumnBase *, NestedColumnAdapter<ESXRecordT>* >::const_iterator iter =
            mAdapters.find (&column);

        if (iter==mAdapters.end())
            throw std::logic_error("No such column in the nestedidadapter");

        return *iter->second;
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void NestedIdCollection<ESXRecordT, IdAccessorT>::addNestedRow(int row, int column, int position)
    {
        Record<ESXRecordT> record;
        record.assign(Collection<ESXRecordT, IdAccessorT>::getRecord(row));

        getAdapter(Collection<ESXRecordT, IdAccessorT>::getColumn(column)).addRow(record, position);

        Collection<ESXRecordT, IdAccessorT>::setRecord(row, record);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void NestedIdCollection<ESXRecordT, IdAccessorT>::removeNestedRows(int row, int column, int subRow)
    {
        Record<ESXRecordT> record;
        record.assign(Collection<ESXRecordT, IdAccessorT>::getRecord(row));

        getAdapter(Collection<ESXRecordT, IdAccessorT>::getColumn(column)).removeRow(record, subRow);

        Collection<ESXRecordT, IdAccessorT>::setRecord(row, record);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    QVariant NestedIdCollection<ESXRecordT, IdAccessorT>::getNestedData (int row,
            int column, int subRow, int subColumn) const
    {
        return getAdapter(Collection<ESXRecordT, IdAccessorT>::getColumn(column)).getData(
                Collection<ESXRecordT, IdAccessorT>::getRecord(row), subRow, subColumn);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void NestedIdCollection<ESXRecordT, IdAccessorT>::setNestedData(int row,
            int column, const QVariant& data, int subRow, int subColumn)
    {
        Record<ESXRecordT> record;
        record.assign(Collection<ESXRecordT, IdAccessorT>::getRecord(row));

        getAdapter(Collection<ESXRecordT, IdAccessorT>::getColumn(column)).setData(
                record, data, subRow, subColumn);

        Collection<ESXRecordT, IdAccessorT>::setRecord(row, record);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    CSMWorld::NestedTableWrapperBase* NestedIdCollection<ESXRecordT, IdAccessorT>::nestedTable(int row,
            int column) const
    {
        return getAdapter(Collection<ESXRecordT, IdAccessorT>::getColumn(column)).table(
                Collection<ESXRecordT, IdAccessorT>::getRecord(row));
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void NestedIdCollection<ESXRecordT, IdAccessorT>::setNestedTable(int row,
            int column, const CSMWorld::NestedTableWrapperBase& nestedTable)
    {
        Record<ESXRecordT> record;
        record.assign(Collection<ESXRecordT, IdAccessorT>::getRecord(row));

        getAdapter(Collection<ESXRecordT, IdAccessorT>::getColumn(column)).setTable(
                record, nestedTable);

        Collection<ESXRecordT, IdAccessorT>::setRecord(row, record);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    int NestedIdCollection<ESXRecordT, IdAccessorT>::getNestedRowsCount(int row, int column) const
    {
        return getAdapter(Collection<ESXRecordT, IdAccessorT>::getColumn(column)).getRowsCount(
                Collection<ESXRecordT, IdAccessorT>::getRecord(row));
    }

    template<typename ESXRecordT, typename IdAccessorT>
    int NestedIdCollection<ESXRecordT, IdAccessorT>::getNestedColumnsCount(int row, int column) const
    {
        const ColumnBase &nestedColumn = Collection<ESXRecordT, IdAccessorT>::getColumn(column);
        int numRecords = Collection<ESXRecordT, IdAccessorT>::getSize();
        if (row >= 0 && row < numRecords)
        {
            const Record<ESXRecordT>& record = Collection<ESXRecordT, IdAccessorT>::getRecord(row);
            return getAdapter(nestedColumn).getColumnsCount(record);
        }
        else
        {
            // If the row is invalid (or there no records), retrieve the column count using a blank record
            const Record<ESXRecordT> record;
            return getAdapter(nestedColumn).getColumnsCount(record);
        }
    }

    template<typename ESXRecordT, typename IdAccessorT>
    CSMWorld::NestableColumn *NestedIdCollection<ESXRecordT, IdAccessorT>::getNestableColumn(int column)
    {
        return Collection<ESXRecordT, IdAccessorT>::getNestableColumn(column);
    }
}

#endif // CSM_WOLRD_NESTEDIDCOLLECTION_H
