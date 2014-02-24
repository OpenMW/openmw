#ifndef CSMSETTINGS_BOOLEANADAPTER_HPP
#define CSMSETTINGS_BOOLEANADAPTER_HPP

#include <QList>
#include <QPair>
#include <QSortFilterProxyModel>

#include "adapter.hpp"
#include "../../view/settings/support.hpp"

class QStandardItemModel;

namespace CSMSettings
{
    class BooleanAdapter : public Adapter
    {
        Q_OBJECT

        QList<QPair<QString, QBool *> > mSettings;

    public:

        explicit BooleanAdapter (QStandardItemModel &model,
                                 CSMSettings::Setting *setting,
                                 QObject *parent = 0);

        int rowCount(const QModelIndex &parent = QModelIndex()) const;

        bool valueExists        (const QString &value) const;
        bool insertValue        (const QString &value);
        bool removeValue        (const QString &value);

        QVariant data           (const QModelIndex &index,
                                 int role = Qt::DisplayRole) const;

        bool setData            (const QModelIndex &index,
                                 const QVariant &value,
                                 int role = Qt::EditRole);
    private:

        bool setSingleValue (const QString &item);
        bool setMultiValue (bool state, const QString &value);

    private slots:

        void slotLayoutChanged();
        void slotDataChanged(const QModelIndex &, const QModelIndex &) {}

    };
}
#endif // CSMSETTINGS_BOOLEANADAPTER_HPP

