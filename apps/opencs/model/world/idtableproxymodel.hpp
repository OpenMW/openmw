#ifndef CSM_WOLRD_IDTABLEPROXYMODEL_H
#define CSM_WOLRD_IDTABLEPROXYMODEL_H

#include <QSortFilterProxyModel>

#include <string>

namespace CSMWorld
{
    class IdTableProxyModel : public QSortFilterProxyModel
    {
            Q_OBJECT

        public:

            IdTableProxyModel (QObject *parent = 0);

            virtual void addRecord (const std::string& id);

            virtual QModelIndex getModelIndex (const std::string& id, int column) const;
    };
}

#endif