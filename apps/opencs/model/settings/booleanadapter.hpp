#ifndef CSMSETTINGS_BOOLEANADAPTER_HPP
#define CSMSETTINGS_BOOLEANADAPTER_HPP

#include <QList>
#include <QPair>
#include <QSortFilterProxyModel>

#include "viewadapter.hpp"
#include "../../view/settings/support.hpp"

namespace CSMSettings
{
    class DefinitionModel;
    class BooleanAdapter : public ViewAdapter
    {
        Q_OBJECT

        QList<QPair<QString, QBool *> > mSettings;

    public:

        explicit BooleanAdapter (DefinitionModel &model,
                                 const CSMSettings::Setting *setting,
                                 QObject *parent = 0);

        int rowCount(const QModelIndex &parent = QModelIndex()) const;
        int columnCount(const QModelIndex &parent = QModelIndex()) const;

        bool valueExists        (const QString &value) const;
        bool insertValue        (const QString &value);
        bool removeValue        (const QString &value);

        QVariant data           (const QModelIndex &index,
                                 int role = Qt::DisplayRole) const;

        bool setData            (const QModelIndex &index,
                                 const QVariant &value,
                                 int role = Qt::EditRole);

        Qt::ItemFlags flags     (const QModelIndex &index) const;

        QModelIndex index(int row, int column, const QModelIndex &parent) const;

        QModelIndex valueIndex (const QString &item, SettingColumn column);

        QModelIndex parent(const QModelIndex &child) const
                                                { return QModelIndex(); }
    private:

        bool setSingleValue (const QString &item);
        bool setMultiValue (bool state, const QString &value);
        bool validIndex (QModelIndex idx) const;
        bool setSourceValue (bool state, const QString &value);

    private slots:
        void slotUpdateData();
        void slotDataChanged (const QModelIndex &, const QModelIndex &);

    };
}
#endif // CSMSETTINGS_BOOLEANADAPTER_HPP
