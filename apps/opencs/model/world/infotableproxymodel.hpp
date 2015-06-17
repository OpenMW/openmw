#ifndef CSM_WORLD_INFOTABLEPROXYMODEL_HPP
#define CSM_WORLD_INFOTABLEPROXYMODEL_HPP

#include <QHash>

#include "idtableproxymodel.hpp"
#include "universalid.hpp"

namespace CSMWorld
{
    class IdTableBase;

    class InfoTableProxyModel : public IdTableProxyModel
    {
            Q_OBJECT

            UniversalId::Type mType;
            IdTableBase *mSourceModel;

            mutable QHash<QString, int> mFirstRowCache;

            int getFirstInfoRow(int currentRow) const;
            ///< Finds the first row with the same topic (journal entry) as in \a currentRow

        public:
            InfoTableProxyModel(UniversalId::Type type, QObject *parent = 0);

            void setSourceModel(QAbstractItemModel *sourceModel);

        protected:
            virtual bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

        private slots:
            void modelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    };
}

#endif
