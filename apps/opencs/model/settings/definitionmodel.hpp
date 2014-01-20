#ifndef CSMSETTINGS_DEFINITIONMODEL_HPP
#define CSMSETTINGS_DEFINITIONMODEL_HPP

#include <QAbstractItemModel>
#include "../../view/settings/support.hpp"

namespace CSMSettings
{
    class DefinitionModel : public QAbstractItemModel
    {
        Q_OBJECT

        DefinitionList mDefinitions;

    public:

        explicit DefinitionModel(QObject *parent = 0);


        int rowCount (const QModelIndex &parent = QModelIndex()) const;
        int columnCount (const QModelIndex &parent = QModelIndex()) const;

        QVariant data (const QModelIndex &index,
                       int role = Qt::DisplayRole) const;

        bool setData (const QModelIndex &index, const QVariant &value,
                      int role = Qt::EditRole);

        Qt::ItemFlags flags (const QModelIndex &index) const;

        QModelIndex index(int row, int column,
                          const QModelIndex &parent = QModelIndex()) const;

        QModelIndex parent(const QModelIndex &child) const
        { return QModelIndex(); }

        bool insertRows(int row, int count, const QModelIndex &parent);
        bool removeRows(int row, int count, const QModelIndex &parent);


        QModelIndex defineSetting ( const QString &settingName,
                                    const QString &pageName,
                                    const QString &value);

        void validate();

        DefinitionListItem getItem (int row) const;

    };
}
#endif // CSMSETTINGS_DEFINITIONMODEL_HPP
