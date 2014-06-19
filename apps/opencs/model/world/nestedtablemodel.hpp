#ifndef CSM_WOLRD_NESTEDTABLEMODEL_H
#define CSM_WOLRD_NESTEDTABLEMODEL_H

#include <vector>

#include <QAbstractProxyModel>

#include "universalid.hpp"
#include "columns.hpp"
#include "columnbase.hpp"

/*! \brief
 * Proxy model used to connect view in the dialogue into the nested columns of the main model.
 */

namespace CSMWorld
{
    class CollectionBase;
    class RecordBase;
    class IdTable;

    class NestedTableModel : public QAbstractProxyModel
    {
        const int mParentColumn;
        IdTable* mMainModel;
        std::string mId;

        public:
        NestedTableModel(const QModelIndex& parent,
                         ColumnBase::Display displayType,
                         IdTable* parentModel);
        //parent is the parent of columns to work with. Columnid provides information about the column

        virtual QModelIndex mapFromSource(const QModelIndex& sourceIndex) const;

        virtual QModelIndex mapToSource(const QModelIndex& proxyIndex) const;

        virtual int rowCount(const QModelIndex& parent) const;

        virtual int columnCount(const QModelIndex& parent) const;

        virtual QModelIndex index(int row, int column, const QModelIndex& parent) const;

        virtual QModelIndex parent(const QModelIndex& index) const;

        virtual QVariant headerData ( int section, Qt::Orientation orientation, int role ) const;
        
    private:
        void setupHeaderVectors(ColumnBase::Display columnId);
    };
}

#endif
