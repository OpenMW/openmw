#ifndef CSM_WOLRD_IDTABLE_H
#define CSM_WOLRD_IDTABLE_H

#include <vector>

#include <QAbstractItemModel>

#include "universalid.hpp"
#include "columns.hpp"

namespace CSMWorld
{
    class CollectionBase;
    class RecordBase;

    class IdTable : public QAbstractItemModel
    {
            Q_OBJECT

        public:

            enum Reordering
            {
                Reordering_None,
                Reordering_WithinTopic
            };

        private:

            CollectionBase *mIdCollection;
            Reordering mReordering;

            // not implemented
            IdTable (const IdTable&);
            IdTable& operator= (const IdTable&);

        public:

            IdTable (CollectionBase *idCollection, Reordering reordering = Reordering_WithinTopic);
            ///< The ownership of \a idCollection is not transferred.

            virtual ~IdTable();

            virtual int rowCount (const QModelIndex & parent = QModelIndex()) const;

            virtual int columnCount (const QModelIndex & parent = QModelIndex()) const;

            virtual QVariant data  (const QModelIndex & index, int role = Qt::DisplayRole) const;

            virtual QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

            virtual bool setData ( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

            virtual Qt::ItemFlags flags (const QModelIndex & index) const;

            virtual bool removeRows (int row, int count, const QModelIndex& parent = QModelIndex());

            virtual QModelIndex index (int row, int column, const QModelIndex& parent = QModelIndex())
                const;

            virtual QModelIndex parent (const QModelIndex& index) const;

            void addRecord (const std::string& id, UniversalId::Type type = UniversalId::Type_None);
            ///< \param type Will be ignored, unless the collection supports multiple record types

            void cloneRecord(const std::string& origin, 
                             const std::string& destination, 
                             UniversalId::Type type = UniversalId::Type_None);

            QModelIndex getModelIndex (const std::string& id, int column) const;

            void setRecord (const std::string& id, const RecordBase& record);
            ///< Add record or overwrite existing recrod.

            const RecordBase& getRecord (const std::string& id) const;

            int searchColumnIndex (Columns::ColumnId id) const;
            ///< Return index of column with the given \a id. If no such column exists, -1 is returned.

            int findColumnIndex (Columns::ColumnId id) const;
            ///< Return index of column with the given \a id. If no such column exists, an exception is
            /// thrown.

            void reorderRows (int baseIndex, const std::vector<int>& newOrder);
            ///< Reorder the rows [baseIndex, baseIndex+newOrder.size()) according to the indices
            /// given in \a newOrder (baseIndex+newOrder[0] specifies the new index of row baseIndex).

            Reordering getReordering() const;
    };
}

#endif
