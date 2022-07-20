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

            ~ResourceTable() override;

            int rowCount (const QModelIndex & parent = QModelIndex()) const override;

            int columnCount (const QModelIndex & parent = QModelIndex()) const override;

            QVariant data  (const QModelIndex & index, int role = Qt::DisplayRole) const override;

            QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

            bool setData ( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

            Qt::ItemFlags flags (const QModelIndex & index) const override;

            QModelIndex index (int row, int column, const QModelIndex& parent = QModelIndex()) const override;

            QModelIndex parent (const QModelIndex& index) const override;

            QModelIndex getModelIndex (const std::string& id, int column) const override;

            /// Return index of column with the given \a id. If no such column exists, -1 is
            /// returned.
            int searchColumnIndex (Columns::ColumnId id) const override;

            /// Return index of column with the given \a id. If no such column exists, an
            /// exception is thrown.
            int findColumnIndex (Columns::ColumnId id) const override;

            /// Return the UniversalId and the hint for viewing \a row. If viewing is not
            /// supported by this table, return (UniversalId::Type_None, "").
            std::pair<UniversalId, std::string> view (int row) const override;

            /// Is \a id flagged as deleted?
            bool isDeleted (const std::string& id) const override;

            int getColumnId (int column) const override;

            /// Signal Qt that the data is about to change.
            void beginReset();
            /// Signal Qt that the data has been changed.
            void endReset();
    };
}

#endif
