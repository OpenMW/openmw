#ifndef CSMSETTINGS_TEXTADAPTER_HPP
#define CSMSETINGS_TEXTADAPTER_HPP

#include "adapter.hpp"

class QStandardItemModel;

namespace CSMSettings
{
    class TextAdapter : public Adapter
    {
        Q_OBJECT

        QString mDelimiter;

    public:

        TextAdapter(QStandardItemModel &model,
                    CSMSettings::Setting *setting,
                    QObject *parent = 0);

        int rowCount (const QModelIndex &parent = QModelIndex()) const;
        int columnCount (const QModelIndex &parent = QModelIndex()) const;

        bool valueExists        (const QString &value) const;
        bool insertValue        (const QString &value);
        bool removeValue        (const QString &value);

        QVariant data           (const QModelIndex &index,
                                 int role = Qt::DisplayRole) const;

        bool setData            (const QModelIndex &index,
                                 const QVariant &value,
                                 int role = Qt::EditRole);

        void setDelimiter (const QString &delimiter)
                                                    { mDelimiter = delimiter;}
    private slots:

        void slotLayoutChanged ();
        void slotDataChanged (const QModelIndex &, const QModelIndex &) {}
    };
}
#endif // CSMSETTINGS_TEXTADAPTER_HPP

