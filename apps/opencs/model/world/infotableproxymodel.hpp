#ifndef CSM_WORLD_INFOTABLEPROXYMODEL_HPP
#define CSM_WORLD_INFOTABLEPROXYMODEL_HPP

#include <QHash>

#include "idtableproxymodel.hpp"
#include "columns.hpp"
#include "universalid.hpp"

namespace CSMWorld
{
    class IdTableBase;

    class InfoTableProxyModel : public IdTableProxyModel
    {
            Q_OBJECT

            UniversalId::Type mType;
            Columns::ColumnId mInfoColumnId;
            ///< Contains ID for Topic or Journal ID
            int mInfoColumnIndex;
            int mLastAddedSourceRow;

            mutable QHash<QString, int> mFirstRowCache;

            int getFirstInfoRow(int currentRow) const;
            ///< Finds the first row with the same topic (journal entry) as in \a currentRow
            ///< \a currentRow is a row of the source model.

        public:
            InfoTableProxyModel(UniversalId::Type type, QObject *parent = nullptr);

            void setSourceModel(QAbstractItemModel *sourceModel) override;

        protected:
            bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

        protected slots:
            void sourceRowsInserted(const QModelIndex &parent, int start, int end) override;
            void sourceRowsRemoved(const QModelIndex &parent, int start, int end) override;
            void sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight) override;
    };
}

#endif
