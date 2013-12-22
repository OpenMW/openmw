#ifndef BINARYWIDGETFILTER_HPP
#define BINARYWIDGETFILTER_HPP

#include <QSortFilterProxyModel>

namespace CSMSettings
{
    class BinaryWidgetFilter : public QSortFilterProxyModel
    {
        Q_OBJECT
        QString mMatchValue;

    public:

        explicit BinaryWidgetFilter(const QString &matchValue, QObject *parent = 0);

        int rowCount        (const QModelIndex &parent = QModelIndex()) const;
        QVariant data       (const QModelIndex &index, int role) const;
        Qt::ItemFlags flags (const QModelIndex &index) const;
        bool setData        (const QModelIndex &index, const QVariant &value,
                             int role = Qt::EditRole);
    };
}
#endif // BINARYWIDGETFILTER_HPP
