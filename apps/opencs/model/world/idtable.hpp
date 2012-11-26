#ifndef CSM_WOLRD_IDTABLE_H
#define CSM_WOLRD_IDTABLE_H

#include <QAbstractTableModel>

namespace CSMWorld
{
    class IdCollectionBase;

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

            int rowCount (const QModelIndex & parent = QModelIndex()) const;

            int columnCount (const QModelIndex & parent = QModelIndex()) const;

            QVariant data  (const QModelIndex & index, int role = Qt::DisplayRole) const;

            QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    };
}

#endif
