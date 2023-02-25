#include "nestedinfocollection.hpp"

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include <apps/opencs/model/world/collection.hpp>
#include <apps/opencs/model/world/columnbase.hpp>
#include <apps/opencs/model/world/info.hpp>
#include <apps/opencs/model/world/infocollection.hpp>
#include <apps/opencs/model/world/nestedcolumnadapter.hpp>
#include <apps/opencs/model/world/nestedtablewrapper.hpp>
#include <apps/opencs/model/world/record.hpp>

namespace CSMWorld
{
    NestedInfoCollection::NestedInfoCollection() {}

    NestedInfoCollection::~NestedInfoCollection()
    {
        for (std::map<const ColumnBase*, NestedColumnAdapter<Info>*>::iterator iter(mAdapters.begin());
             iter != mAdapters.end(); ++iter)
        {
            delete (*iter).second;
        }
    }

    void NestedInfoCollection::addAdapter(std::pair<const ColumnBase*, NestedColumnAdapter<Info>*> adapter)
    {
        mAdapters.insert(adapter);
    }

    const NestedColumnAdapter<Info>& NestedInfoCollection::getAdapter(const ColumnBase& column) const
    {
        std::map<const ColumnBase*, NestedColumnAdapter<Info>*>::const_iterator iter = mAdapters.find(&column);

        if (iter == mAdapters.end())
            throw std::logic_error("No such column in the nestedidadapter");

        return *iter->second;
    }

    void NestedInfoCollection::addNestedRow(int row, int column, int position)
    {
        auto record = std::make_unique<Record<Info>>();
        record->assign(Collection<Info>::getRecord(row));

        getAdapter(Collection<Info>::getColumn(column)).addRow(*record, position);

        Collection<Info>::setRecord(row, std::move(record));
    }

    void NestedInfoCollection::removeNestedRows(int row, int column, int subRow)
    {
        auto record = std::make_unique<Record<Info>>();
        record->assign(Collection<Info>::getRecord(row));

        getAdapter(Collection<Info>::getColumn(column)).removeRow(*record, subRow);

        Collection<Info>::setRecord(row, std::move(record));
    }

    QVariant NestedInfoCollection::getNestedData(int row, int column, int subRow, int subColumn) const
    {
        return getAdapter(Collection<Info>::getColumn(column))
            .getData(Collection<Info>::getRecord(row), subRow, subColumn);
    }

    void NestedInfoCollection::setNestedData(int row, int column, const QVariant& data, int subRow, int subColumn)
    {
        auto record = std::make_unique<Record<Info>>();
        record->assign(Collection<Info>::getRecord(row));

        getAdapter(Collection<Info>::getColumn(column)).setData(*record, data, subRow, subColumn);

        Collection<Info>::setRecord(row, std::move(record));
    }

    CSMWorld::NestedTableWrapperBase* NestedInfoCollection::nestedTable(int row, int column) const
    {
        return getAdapter(Collection<Info>::getColumn(column)).table(Collection<Info>::getRecord(row));
    }

    void NestedInfoCollection::setNestedTable(int row, int column, const CSMWorld::NestedTableWrapperBase& nestedTable)
    {
        auto record = std::make_unique<Record<Info>>();
        record->assign(Collection<Info>::getRecord(row));

        getAdapter(Collection<Info>::getColumn(column)).setTable(*record, nestedTable);

        Collection<Info>::setRecord(row, std::move(record));
    }

    int NestedInfoCollection::getNestedRowsCount(int row, int column) const
    {
        return getAdapter(Collection<Info>::getColumn(column)).getRowsCount(Collection<Info>::getRecord(row));
    }

    int NestedInfoCollection::getNestedColumnsCount(int row, int column) const
    {
        return getAdapter(Collection<Info>::getColumn(column)).getColumnsCount(Collection<Info>::getRecord(row));
    }

    CSMWorld::NestableColumn* NestedInfoCollection::getNestableColumn(int column)
    {
        return Collection<Info>::getNestableColumn(column);
    }
}
