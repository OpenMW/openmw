#ifndef CSM_WOLRD_SUBCOLLECTION_H
#define CSM_WOLRD_SUBCOLLECTION_H

#include <map>
#include <stdexcept>

#include <components/esm/loadpgrd.hpp>

#include "columnimp.hpp"
#include "idcollection.hpp"
#include "nestedcollection.hpp"
#include "nestedtablewrapper.hpp"
#include "idadapterimp.hpp"

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
    class SubCellCollection : public IdCollection<ESXRecordT, IdAccessorT>, public NestedCollection
    {
            const IdCollection<Cell>& mCells;
            std::map<const ColumnBase*, NestedIdAdapter<ESXRecordT>* > mAdapters;

            virtual void loadRecord (ESXRecordT& record, ESM::ESMReader& reader);

            NestedIdAdapter<ESXRecordT>* getAdapter(const ColumnBase &column) const;

        public:

            SubCellCollection (const IdCollection<Cell>& cells);
            ~SubCellCollection();

            virtual void addNestedRow(int row, int column, int position);

            virtual void removeNestedRows(int row, int column, int subRow);

            virtual QVariant getNestedData(int row, int column, int subRow, int subColumn) const;

            virtual void setNestedData(int row, int column, const QVariant& data, int subRow, int subColumn);

            virtual NestedTableWrapperBase* nestedTable(int row, int column) const;

            virtual void setNestedTable(int row, int column, const NestedTableWrapperBase& nestedTable);

            virtual int getNestedRowsCount(int row, int column) const;

            virtual int getNestedColumnsCount(int row, int column) const;

            // this method is inherited from NestedCollection, not from Collection<ESXRecordT>
            virtual NestableColumn *getNestableColumn(int column);

            void addAdapter(std::pair<const ColumnBase*, NestedIdAdapter<ESXRecordT>* > adapter);
    };

    template<typename ESXRecordT, typename IdAccessorT>
    void SubCellCollection<ESXRecordT, IdAccessorT>::loadRecord (ESXRecordT& record,
        ESM::ESMReader& reader)
    {
        record.load (reader, mCells);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    SubCellCollection<ESXRecordT, IdAccessorT>::SubCellCollection (const IdCollection<Cell>& cells)
    : mCells (cells)
    {}

    template<typename ESXRecordT, typename IdAccessorT>
    SubCellCollection<ESXRecordT, IdAccessorT>::~SubCellCollection()
    {
        for (std::map<const ColumnBase *, NestedIdAdapter<ESXRecordT>* >::iterator iter (mAdapters.begin());
                iter!=mAdapters.end(); ++iter)
            delete (*iter).second;
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void SubCellCollection<ESXRecordT, IdAccessorT>::addAdapter(std::pair<const ColumnBase*, NestedIdAdapter<ESXRecordT>* > adapter)
    {
        mAdapters.insert(adapter);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    NestedIdAdapter<ESXRecordT>* SubCellCollection<ESXRecordT, IdAccessorT>::getAdapter(const ColumnBase &column) const
    {
        std::map<const ColumnBase *, NestedIdAdapter<ESXRecordT>* >::const_iterator iter =
            mAdapters.find (&column);

        if (iter==mAdapters.end())
            throw std::logic_error("No such column in the nestedidadapter");

        return iter->second;
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void SubCellCollection<ESXRecordT, IdAccessorT>::addNestedRow(int row, int column, int position)
    {
        getAdapter(getColumn(column))->addNestedRow(getRecord(row), position);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void SubCellCollection<ESXRecordT, IdAccessorT>::removeNestedRows(int row, int column, int subRow)
    {
        getAdapter(getColumn(column))->removeNestedRow(getRecord(row), subRow);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    QVariant SubCellCollection<ESXRecordT, IdAccessorT>::getNestedData (int row,
            int column, int subRow, int subColumn) const
    {
        return getAdapter(getColumn(column))->getNestedData(getRecord(row), subRow, subColumn);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void SubCellCollection<ESXRecordT, IdAccessorT>::setNestedData(int row,
            int column, const QVariant& data, int subRow, int subColumn)
    {
        getAdapter(getColumn(column))->setNestedData(getRecord(row), data, subRow, subColumn);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    CSMWorld::NestedTableWrapperBase* SubCellCollection<ESXRecordT, IdAccessorT>::nestedTable(int row,
            int column) const
    {
        return getAdapter(getColumn(column))->nestedTable(getRecord(row));
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void SubCellCollection<ESXRecordT, IdAccessorT>::setNestedTable(int row,
            int column, const CSMWorld::NestedTableWrapperBase& nestedTable)
    {
        getAdapter(getColumn(column))->setNestedTable(getRecord(row), nestedTable);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    int SubCellCollection<ESXRecordT, IdAccessorT>::getNestedRowsCount(int row, int column) const
    {
        return getAdapter(getColumn(column))->getNestedRowsCount(getRecord(row));
    }

    template<typename ESXRecordT, typename IdAccessorT>
    int SubCellCollection<ESXRecordT, IdAccessorT>::getNestedColumnsCount(int row, int column) const
    {
        return getAdapter(getColumn(column))->getNestedColumnsCount(getRecord(row));
    }

    template<typename ESXRecordT, typename IdAccessorT>
    CSMWorld::NestableColumn *SubCellCollection<ESXRecordT, IdAccessorT>::getNestableColumn(int column)
    {
        return Collection::getNestableColumn(column);
    }
}

#endif
