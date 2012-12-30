#ifndef CSM_WOLRD_IDTABLE_H
#define CSM_WOLRD_IDTABLE_H

#include <QAbstractTableModel>

namespace CSMWorld
{
    class IdCollectionBase;
    class RecordBase;

    class IdTable : public QAbstractTableModel
    {
            Q_OBJECT

            IdCollectionBase *mIdCollection;

            // not implemented
            IdTable (const IdTable&);
            IdTable& operator= (const IdTable&);

        public:

            IdTable (IdCollectionBase *idCollection);
            ///< The ownership of \a idCollection is not transferred.

            virtual ~IdTable();

            virtual int rowCount (const QModelIndex & parent = QModelIndex()) const;

            virtual int columnCount (const QModelIndex & parent = QModelIndex()) const;

            virtual QVariant data  (const QModelIndex & index, int role = Qt::DisplayRole) const;

            virtual QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

            virtual bool setData ( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

            virtual Qt::ItemFlags flags (const QModelIndex & index) const;

            virtual bool removeRows (int row, int count, const QModelIndex& parent = QModelIndex());

            void addRecord (const std::string& id);

            QModelIndex getModelIndex (const std::string& id, int column) const;

            void setRecord (const RecordBase& record);
            ///< Add record or overwrite existing recrod.

            const RecordBase& getRecord (const std::string& id) const;
    };
}

#endif
