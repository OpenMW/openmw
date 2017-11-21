#ifndef CSM_WOLRD_RESOURCETABLE_H
#define CSM_WOLRD_RESOURCETABLE_H

#include "idtablebase.hpp"

namespace CSMWorld
{
    class Resources;

    class ResourceTable : public IdTableBase
    {
            const Resources *mResources;

        public:

            /// \note The feature Feature_Constant will be added implicitly.
            ResourceTable (const Resources *resources, unsigned int features = 0);

            virtual ~ResourceTable();

            virtual int rowCount (const QModelIndex & parent = QModelIndex()) const;

            virtual int columnCount (const QModelIndex & parent = QModelIndex()) const;

            virtual QVariant data  (const QModelIndex & index, int role = Qt::DisplayRole) const;

            virtual QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

            virtual bool setData ( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

            virtual Qt::ItemFlags flags (const QModelIndex & index) const;

            virtual QModelIndex index (int row, int column, const QModelIndex& parent = QModelIndex())
                const;

            virtual QModelIndex parent (const QModelIndex& index) const;

            virtual QModelIndex getModelIndex (const std::string& id, int column) const;

            /// Return index of column with the given \a id. If no such column exists, -1 is
            /// returned.
            virtual int searchColumnIndex (Columns::ColumnId id) const;

            /// Return index of column with the given \a id. If no such column exists, an
            /// exception is thrown.
            virtual int findColumnIndex (Columns::ColumnId id) const;

            /// Return the UniversalId and the hint for viewing \a row. If viewing is not
            /// supported by this table, return (UniversalId::Type_None, "").
            virtual std::pair<UniversalId, std::string> view (int row) const;

            /// Is \a id flagged as deleted?
            virtual bool isDeleted (const std::string& id) const;

            virtual int getColumnId (int column) const;

            /// Signal Qt that the data is about to change.
            void beginReset();
            /// Signal Qt that the data has been changed.
            void endReset();
    };
}

#endif
