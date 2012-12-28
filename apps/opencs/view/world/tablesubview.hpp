#ifndef CSV_WORLD_TABLESUBVIEW_H
#define CSV_WORLD_TABLESUBVIEW_H

#include "../doc/subview.hpp"

class QModelIndex;

namespace CSMDoc
{
    class Document;
}

namespace CSVWorld
{
    class Table;

    class TableSubView : public CSVDoc::SubView
    {
            Q_OBJECT

            Table *mTable;

        public:

            TableSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document, bool createAndDelete);

            virtual void setEditLock (bool locked);

        private slots:

            void rowActivated (const QModelIndex& index);
    };
}

#endif