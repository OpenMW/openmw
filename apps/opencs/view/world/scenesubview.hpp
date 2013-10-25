#ifndef CSV_WORLD_SCENESUBVIEW_H
#define CSV_WORLD_SCENESUBVIEW_H

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

    class SceneSubView : public CSVDoc::SubView
    {
            Q_OBJECT

            TableBottomBox *mBottom;

        public:

            SceneSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document);

            virtual void setEditLock (bool locked);

            virtual void updateEditorSetting (const QString& key, const QString& value);

            virtual void setStatusBar (bool show);
    };
}

#endif
