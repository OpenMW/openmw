#ifndef CSV_WORLD_SCENESUBVIEW_H
#define CSV_WORLD_SCENESUBVIEW_H

#include "../doc/subview.hpp"

class QModelIndex;

namespace CSMDoc
{
    class Document;
}

namespace CSVRender
{
    class WorldspaceWidget;
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
            CSVRender::WorldspaceWidget *mScene;

        public:

            SceneSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document);

            virtual void setEditLock (bool locked);

            virtual void updateUserSetting
                                (const QString& key, const QStringList &list);

            virtual void setStatusBar (bool show);

        private slots:

            void closeRequest();
    };
}

#endif
