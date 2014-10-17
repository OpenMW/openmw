#ifndef CSM_WOLRD_IDTABLEPROXYMODEL_H
#define CSM_WOLRD_IDTABLEPROXYMODEL_H

#include <string>

#include <boost/shared_ptr.hpp>

#include <map>

#include <QSortFilterProxyModel>

#include "../filter/node.hpp"

namespace CSMWorld
{
    class IdTableProxyModel : public QSortFilterProxyModel
    {
            Q_OBJECT

            boost::shared_ptr<CSMFilter::Node> mFilter;
            std::map<int, int> mColumnMap; // column ID, column index in this model (or -1)

        private:

            void updateColumnMap();

            bool filterAcceptsRow (int sourceRow, const QModelIndex& sourceParent) const;

        public:

            IdTableProxyModel (QObject *parent = 0);

            virtual QModelIndex getModelIndex (const std::string& id, int column) const;

            void setFilter (const boost::shared_ptr<CSMFilter::Node>& filter);

        protected:

            bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
    };
}

#endif
