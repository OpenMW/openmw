#ifndef CSM_WOLRD_IDTABLEBASE_H
#define CSM_WOLRD_IDTABLEBASE_H

#include <QAbstractItemModel>

#include "columns.hpp"

namespace CSMWorld
{
    class UniversalId;

    class IdTableBase : public QAbstractItemModel
    {
            Q_OBJECT

        public:

            enum Features
            {
                Feature_ReorderWithinTopic = 1,

                /// Use ID column to generate view request (ID is transformed into
                /// worldspace and original ID is passed as hint with c: prefix).
                Feature_ViewId = 2,

                /// Use cell column to generate view request (cell ID is transformed
                /// into worldspace and record ID is passed as hint with r: prefix).
                Feature_ViewCell = 4,

                Feature_View = Feature_ViewId | Feature_ViewCell,

                Feature_Preview = 8,

                /// Table can not be modified through ordinary means.
                Feature_Constant = 16,

                Feature_AllowTouch = 32
            };

        private:

            unsigned int mFeatures;

        public:

            IdTableBase (unsigned int features);

            virtual QModelIndex getModelIndex (const std::string& id, int column) const = 0;

            /// Return index of column with the given \a id. If no such column exists, -1 is
            /// returned.
            virtual int searchColumnIndex (Columns::ColumnId id) const = 0;

            /// Return index of column with the given \a id. If no such column exists, an
            /// exception is thrown.
            virtual int findColumnIndex (Columns::ColumnId id) const = 0;

            /// Return the UniversalId and the hint for viewing \a row. If viewing is not
            /// supported by this table, return (UniversalId::Type_None, "").
            virtual std::pair<UniversalId, std::string> view (int row) const = 0;

            /// Is \a id flagged as deleted?
            virtual bool isDeleted (const std::string& id) const = 0;

            virtual int getColumnId (int column) const = 0;

            unsigned int getFeatures() const;
    };
}

#endif
