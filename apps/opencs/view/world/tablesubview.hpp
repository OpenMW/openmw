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
    class TableBottomBox;
    class CreatorFactoryBase;

    class TableSubView : public CSVDoc::SubView
    {
            Q_OBJECT

            Table *mTable;
            TableBottomBox *mBottom;

        public:

            TableSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document,
                const CreatorFactoryBase& creatorFactory, bool sorting);

            virtual void setEditLock (bool locked);

            virtual void updateEditorSetting (const QString& key, const QString& value);

            virtual void setStatusBar (bool show);

        private slots:

            void editRequest (int row);
    };
}

#endif
