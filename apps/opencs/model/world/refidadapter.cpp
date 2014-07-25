#include "refidadapter.hpp"

#include "nestedtablewrapper.hpp"

#include <QVariant>

CSMWorld::RefIdAdapter::RefIdAdapter() {}

CSMWorld::RefIdAdapter::~RefIdAdapter() {}

CSMWorld::NestedRefIdAdapterBase::NestedRefIdAdapterBase() {}

CSMWorld::NestedRefIdAdapterBase::~NestedRefIdAdapterBase() {}

CSMWorld::NestedRefIdAdapter::NestedRefIdAdapter()
{}

CSMWorld::NestedRefIdAdapter::~NestedRefIdAdapter()
{
    for (unsigned i = 0; i < mAssociatedColumns.size(); ++i)
    {
        delete mAssociatedColumns[i].second;
    }
}

void CSMWorld::NestedRefIdAdapter::setNestedData (const RefIdColumn *column, RefIdData& data, int row,
                                             const QVariant& value, int subRowIndex, int subColIndex) const
{
    getHelper(column)->setNestedData(data, row, value, subRowIndex, subColIndex);
}

QVariant CSMWorld::NestedRefIdAdapter::getNestedData(const RefIdColumn *column, const RefIdData& data,
                                                     int index, int subRowIndex, int subColIndex) const
{
    return getHelper(column)->getNestedData(data, index, subRowIndex, subColIndex);
}

int CSMWorld::NestedRefIdAdapter::getNestedColumnsCount(const RefIdColumn *column, const RefIdData& data) const
{
    return getHelper(column)->getNestedColumnsCount(data);
}


int CSMWorld::NestedRefIdAdapter::getNestedRowsCount(const RefIdColumn *column, const RefIdData& data, int index) const
{
    return getHelper(column)->getNestedRowsCount(data, index);
}


void CSMWorld::NestedRefIdAdapter::removeNestedRow (const RefIdColumn *column, RefIdData& data, int index, int rowToRemove) const
{
    getHelper(column)->removeNestedRow(data, index, rowToRemove);
}

void CSMWorld::NestedRefIdAdapter::addNestedRow (const RefIdColumn *column, RefIdData& data, int index, int position) const
{
    getHelper(column)->addNestedRow(data, index, position); //This code grows more boring and boring. I would love some macros.
}

void CSMWorld::NestedRefIdAdapter::setNestedTable (const RefIdColumn* column, RefIdData& data, int index, const NestedTableWrapperBase& nestedTable)
{
    getHelper(column)->setNestedTable(data, index, nestedTable);
}


CSMWorld::NestedTableWrapperBase* CSMWorld::NestedRefIdAdapter::nestedTable (const RefIdColumn* column, const RefIdData& data, int index) const
{
    return getHelper(column)->nestedTable(data, index);
}

CSMWorld::HelperBase* CSMWorld::NestedRefIdAdapter::getHelper(const RefIdColumn *column) const
{
    for (unsigned i = 0; i < mAssociatedColumns.size(); ++i)
    {
        if (mAssociatedColumns[i].first == column)
        {
            return mAssociatedColumns[i].second;
        }
    }

    throw std::logic_error("No such column in the nestedrefidadapter");

    return NULL;
}

void CSMWorld::NestedRefIdAdapter::setAssocColumns(const std::vector<std::pair <const RefIdColumn*, HelperBase*> >& assocColumns)
{
    mAssociatedColumns = assocColumns;
}

void CSMWorld::NestedRefIdAdapter::addAssocColumn(const std::pair <const RefIdColumn*, HelperBase*>& assocColumn)
{
    mAssociatedColumns.push_back(assocColumn);
}
