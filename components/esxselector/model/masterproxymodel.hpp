#ifndef MASTERPROXYMODEL_HPP
#define MASTERPROXYMODEL_HPP

#include <QSortFilterProxyModel>

class QAbstractTableModel;

namespace EsxSelector
{
    class MasterProxyModel : public QSortFilterProxyModel
    {
        Q_OBJECT
    public:
        explicit MasterProxyModel(QObject *parent = 0, QAbstractTableModel *model = 0);
        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    signals:

    public slots:

    };
}
#endif // MASTERPROXYMODEL_HPP
