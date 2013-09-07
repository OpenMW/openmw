#ifndef MASTERPROXYMODEL_HPP
#define MASTERPROXYMODEL_HPP

#include <QSortFilterProxyModel>
#include <QStringList>
#include <QMimeData>

class QAbstractTableModel;

namespace EsxModel
{
    class MasterProxyModel : public QSortFilterProxyModel
    {
        Q_OBJECT
    public:
        explicit MasterProxyModel(QObject *parent = 0, QAbstractTableModel *model = 0);
     //   virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    signals:

    public slots:
        void slotSourceModelChanged(QModelIndex topLeft, QModelIndex botRight);
    };
}
#endif // MASTERPROXYMODEL_HPP
