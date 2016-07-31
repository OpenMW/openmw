#include "nestedinfocollection.hpp"

#include "nestedcoladapterimp.hpp"

namespace CSMWorld
{
    NestedInfoCollection::NestedInfoCollection ()
    {}

    NestedInfoCollection::~NestedInfoCollection()
    {
        for (std::map<const ColumnBase *, NestedColumnAdapter<Info>* >::iterator
                iter (mAdapters.begin()); iter!=mAdapters.end(); ++iter)
        {
            delete (*iter).second;
        }
    }

    void NestedInfoCollection::addAdapter(std::pair<const ColumnBase*,
            NestedColumnAdapter<Info>* > adapter)
    {
        mAdapters.insert(adapter);
    }

    const NestedColumnAdapter<Info>& NestedInfoCollection::getAdapter(const ColumnBase &column) const
    {
        std::map<const ColumnBase *, NestedColumnAdapter<Info>* >::const_iterator iter =
            mAdapters.find (&column);

        if (iter==mAdapters.end())
            throw std::logic_error("No such column in the nestedidadapter");

        return *iter->second;
    }

    void NestedInfoCollection::addNestedRow(int row, int column, int position)
    {
        Record<Info> record;
        record.assign(Collection<Info, IdAccessor<Info> >::getRecord(row));

        getAdapter(Collection<Info, IdAccessor<Info> >::getColumn(column)).addRow(record, position);

        Collection<Info, IdAccessor<Info> >::setRecord(row, record);
    }

    void NestedInfoCollection::removeNestedRows(int row, int column, int subRow)
    {
        Record<Info> record;
        record.assign(Collection<Info, IdAccessor<Info> >::getRecord(row));

        getAdapter(Collection<Info, IdAccessor<Info> >::getColumn(column)).removeRow(record, subRow);

        Collection<Info, IdAccessor<Info> >::setRecord(row, record);
    }

    QVariant NestedInfoCollection::getNestedData (int row,
            int column, int subRow, int subColumn) const
    {
        return getAdapter(Collection<Info, IdAccessor<Info> >::getColumn(column)).getData(
                Collection<Info, IdAccessor<Info> >::getRecord(row), subRow, subColumn);
    }

    void NestedInfoCollection::setNestedData(int row,
            int column, const QVariant& data, int subRow, int subColumn)
    {
        Record<Info> record;
        record.assign(Collection<Info, IdAccessor<Info> >::getRecord(row));

        getAdapter(Collection<Info, IdAccessor<Info> >::getColumn(column)).setData(
                record, data, subRow, subColumn);

        Collection<Info, IdAccessor<Info> >::setRecord(row, record);
    }

    CSMWorld::NestedTableWrapperBase* NestedInfoCollection::nestedTable(int row,
            int column) const
    {
        return getAdapter(Collection<Info, IdAccessor<Info> >::getColumn(column)).table(
                Collection<Info, IdAccessor<Info> >::getRecord(row));
    }

    void NestedInfoCollection::setNestedTable(int row,
            int column, const CSMWorld::NestedTableWrapperBase& nestedTable)
    {
        Record<Info> record;
        record.assign(Collection<Info, IdAccessor<Info> >::getRecord(row));

        getAdapter(Collection<Info, IdAccessor<Info> >::getColumn(column)).setTable(
                record, nestedTable);

        Collection<Info, IdAccessor<Info> >::setRecord(row, record);
    }

    int NestedInfoCollection::getNestedRowsCount(int row, int column) const
    {
        return getAdapter(Collection<Info, IdAccessor<Info> >::getColumn(column)).getRowsCount(
                Collection<Info, IdAccessor<Info> >::getRecord(row));
    }

    int NestedInfoCollection::getNestedColumnsCount(int row, int column) const
    {
        return getAdapter(Collection<Info, IdAccessor<Info> >::getColumn(column)).getColumnsCount(
                Collection<Info, IdAccessor<Info> >::getRecord(row));
    }

    CSMWorld::NestableColumn *NestedInfoCollection::getNestableColumn(int column)
    {
        return Collection<Info, IdAccessor<Info> >::getNestableColumn(column);
    }
}
