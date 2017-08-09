#ifndef CSM_WOLRD_IDTREE_H
#define CSM_WOLRD_IDTREE_H

#include "idtable.hpp"
#include "universalid.hpp"
#include "columns.hpp"

/*! \brief
 * Class for holding the model. Uses typical qt table abstraction/interface for granting access
 * to the individiual fields of the records, Some records are holding nested data (for instance
 * inventory list of the npc). In cases like this, table model offers interface to access
 * nested data in the qt way - that is specify parent. Since some of those nested data require
 * multiple columns to represent information, single int (default way to index model in the
 * qmodelindex) is not sufficiant. Therefore tablemodelindex class can hold two ints for the
 * sake of indexing two dimensions of the table. This model does not support multiple levels of
 * the nested data. Vast majority of methods makes sense only for the top level data.
 */

namespace CSMWorld
{
    class NestedCollection;
    struct RecordBase;
    struct NestedTableWrapperBase;

    class IdTree : public IdTable
    {
            Q_OBJECT

        private:

            NestedCollection *mNestedCollection;

            // not implemented
            IdTree (const IdTree&);
            IdTree& operator= (const IdTree&);

            unsigned int foldIndexAddress(const QModelIndex& index) const;
            std::pair<int, int> unfoldIndexAddress(unsigned int id) const;

        public:

            IdTree (NestedCollection *nestedCollection, CollectionBase *idCollection, unsigned int features = 0);
            ///< The ownerships of \a nestedCollecton and \a idCollection are not transferred.

            virtual ~IdTree();

            virtual int rowCount (const QModelIndex & parent = QModelIndex()) const;

            virtual int columnCount (const QModelIndex & parent = QModelIndex()) const;

            virtual QVariant data  (const QModelIndex & index, int role = Qt::DisplayRole) const;

            virtual bool setData ( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

            virtual Qt::ItemFlags flags (const QModelIndex & index) const;

            virtual bool removeRows (int row, int count, const QModelIndex& parent = QModelIndex());

            virtual QModelIndex index (int row, int column, const QModelIndex& parent = QModelIndex())
                const;

            virtual QModelIndex parent (const QModelIndex& index) const;

            QModelIndex getNestedModelIndex (const std::string& id, int column) const;

            QVariant nestedHeaderData(int section, int subSection, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

            NestedTableWrapperBase* nestedTable(const QModelIndex &index) const;

            void setNestedTable(const QModelIndex &index, const NestedTableWrapperBase& nestedTable);

            void addNestedRow (const QModelIndex& parent, int position);

            virtual bool hasChildren (const QModelIndex& index) const;

            virtual int searchNestedColumnIndex(int parentColumn, Columns::ColumnId id);
            ///< \return the column index or -1 if the requested column wasn't found.

            virtual int findNestedColumnIndex(int parentColumn, Columns::ColumnId id);
            ///< \return the column index or throws an exception if the requested column wasn't found.

    signals:

        void resetStart(const QString& id);

        void resetEnd(const QString& id);
    };
}

#endif
