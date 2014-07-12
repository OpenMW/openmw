#ifndef CSM_WOLRD_IDTABLE_H
#define CSM_WOLRD_IDTABLE_H

#include <vector>

#include "idtablebase.hpp"
#include "universalid.hpp"
#include "columns.hpp"

/*! \brief
 * Clas for holding the model. Uses typical qt table abstraction/interface for granting access to the individiual fields of the records,
 * Some records are holding nested data (for instance inventory list of the npc). In casses like this, table model offers interface
 * to access nested data in the qt way – that is specify parent. Since some of those nested data require multiple columns to
 * represent informations, single int (default way to index model in the qmodelindex) is not sufficiant. Therefore tablemodelindex class
 * can hold two ints for the sake of indexing two dimensions of the table. This model does not support multiple levels of the nested
 * data. Vast majority of methods makes sense only for the top level data.
 */

namespace CSMWorld
{
    class CollectionBase;
    class RecordBase;

    class IdTable : public IdTableBase
    {
            Q_OBJECT

        private:

            CollectionBase *mIdCollection;

            // not implemented
            IdTable (const IdTable&);
            IdTable& operator= (const IdTable&);
            unsigned int foldIndexAdress(const QModelIndex& index) const;
            std::pair<int, int> unfoldIndexAdress(unsigned int id) const;

        public:

            IdTable (CollectionBase *idCollection, unsigned int features = 0);
            ///< The ownership of \a idCollection is not transferred.

            virtual ~IdTable();

            virtual int rowCount (const QModelIndex & parent = QModelIndex()) const;

            virtual int columnCount (const QModelIndex & parent = QModelIndex()) const;

            virtual QVariant data  (const QModelIndex & index, int role = Qt::DisplayRole) const;

            virtual QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

            QVariant nestedHeaderData(int section, int subSection, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

            virtual bool setData ( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

            virtual Qt::ItemFlags flags (const QModelIndex & index) const;

            virtual bool removeRows (int row, int count, const QModelIndex& parent = QModelIndex());

            void addNestedRow (const QModelIndex& parent, int position);

            virtual QModelIndex index (int row, int column, const QModelIndex& parent = QModelIndex())
                const;

            virtual QModelIndex parent (const QModelIndex& index) const;

            virtual bool hasChildren (const QModelIndex& index) const;

            void addRecord (const std::string& id, UniversalId::Type type = UniversalId::Type_None);
            ///< \param type Will be ignored, unless the collection supports multiple record types

            void cloneRecord(const std::string& origin,
                             const std::string& destination,
                             UniversalId::Type type = UniversalId::Type_None);

            virtual QModelIndex getModelIndex (const std::string& id, int column) const;

            void setRecord (const std::string& id, const RecordBase& record);
            ///< Add record or overwrite existing recrod.

            const RecordBase& getRecord (const std::string& id) const;

            virtual int searchColumnIndex (Columns::ColumnId id) const;
            ///< Return index of column with the given \a id. If no such column exists, -1 is returned.

            virtual int findColumnIndex (Columns::ColumnId id) const;
            ///< Return index of column with the given \a id. If no such column exists, an exception is
            /// thrown.

            void reorderRows (int baseIndex, const std::vector<int>& newOrder);
            ///< Reorder the rows [baseIndex, baseIndex+newOrder.size()) according to the indices
            /// given in \a newOrder (baseIndex+newOrder[0] specifies the new index of row baseIndex).

            virtual std::pair<UniversalId, std::string> view (int row) const;
            ///< Return the UniversalId and the hint for viewing \a row. If viewing is not
            /// supported by this table, return (UniversalId::Type_None, "").

            /// Is \a id flagged as deleted?
            virtual bool isDeleted (const std::string& id) const;

            int getColumnId(int column) const;
    };
}

#endif
