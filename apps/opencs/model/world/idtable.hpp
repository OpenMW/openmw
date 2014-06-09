#ifndef CSM_WOLRD_IDTABLE_H
#define CSM_WOLRD_IDTABLE_H

#include <vector>

#include <QAbstractItemModel>

#include "universalid.hpp"
#include "columns.hpp"

<<<<<<< Updated upstream
=======
/*! \brief
 * Clas for holding the model. Uses typical qt table abstraction/interface for granting access to the individiual fields of the records,
 * Some records are holding nested data (for instance inventory list of the npc). In casses like this, table model offers interface
 * to access nested data in the qt way – that is specify parent. The parent is encoded in the internalid of the index model
 * See methods fold and unfold adress to see why. This approach has some serious limitations: it allows only
 * a single level of the nesting. At the point of creating this code this seemed to be a good enough solution.
 * If for some reason it turned out that in fact multiple levels of nesting are needed, change in the addressing of the
 * index is most likely the very first to be considered.
 */

>>>>>>> Stashed changes
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

            enum Viewing
            {
                Viewing_None,
                Viewing_Id, // use ID column to generate view request (ID is transformed into
                            // worldspace and original ID is passed as hint with c: prefix)
                Viewing_Cell // use cell column to generate view request (cell ID is transformed
                             // into worldspace and record ID is passed as hint with r: prefix)
            };

        private:

            CollectionBase *mIdCollection;
            Reordering mReordering;
            Viewing mViewing;
            bool mPreview;

            // not implemented
            IdTable (const IdTable&);
            IdTable& operator= (const IdTable&);
<<<<<<< Updated upstream
=======

            unsigned int foldIndexAdress(const QModelIndex& index) const;

            std::pair<int, int> unfoldIndexAdress(unsigned int id) const;
>>>>>>> Stashed changes

        public:

            IdTable (CollectionBase *idCollection, Reordering reordering = Reordering_None,
                Viewing viewing = Viewing_None, bool preview = false);
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
	
	    virtual bool hasChildren (const QModelIndex& index) const;

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

            Viewing getViewing() const;

            bool hasPreview() const;

            std::pair<UniversalId, std::string> view (int row) const;
            ///< Return the UniversalId and the hint for viewing \a row. If viewing is not
            /// supported by this table, return (UniversalId::Type_None, "").

            int getColumnId(int column) const;
    };
}

#endif
