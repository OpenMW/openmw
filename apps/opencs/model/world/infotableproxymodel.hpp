#ifndef CSM_WORLD_INFOTABLEPROXYMODEL_HPP
#define CSM_WORLD_INFOTABLEPROXYMODEL_HPP

#include "idtableproxymodel.hpp"
#include "universalid.hpp"

namespace CSMWorld
{
    class IdTableBase;

    class InfoTableProxyModel : public IdTableProxyModel
    {
            UniversalId::Type mType;
            IdTableBase *mSourceModel;

            int getFirstInfoRow(int currentRow) const;
            ///< Finds the first row with the same topic (journal entry) as in \a currentRow

        public:
            InfoTableProxyModel(UniversalId::Type type, QObject *parent = 0);

            void setSourceModel(QAbstractItemModel *sourceModel);

        protected:
            virtual bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
    };
}

#endif
