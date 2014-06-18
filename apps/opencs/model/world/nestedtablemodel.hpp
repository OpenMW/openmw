 
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
        std::string mId;
        std::vector<std::string> mHeaderTitle;
        std::vector<ColumnBase::Display> mHeaderDisplay;
        
        public:
        NestedTableModel(const QModelIndex& parent,
                         ColumnBase::Display displayType,
                         IdTable* parentModel);
        //parent is the parent of columns to work with. Columnid provides information about the column
        
        virtual QModelIndex mapFromSource(const QModelIndex& sourceIndex) const;
        
        virtual QModelIndex mapToSource(const QModelIndex& proxyIndex) const;
    };
}

#endif
