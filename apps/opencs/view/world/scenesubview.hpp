#ifndef CSV_WORLD_SCENESUBVIEW_H
#define CSV_WORLD_SCENESUBVIEW_H

#include "../doc/subview.hpp"

#include "../render/navigation1st.hpp"
#include "../render/navigationfree.hpp"
#include "../render/navigationorbit.hpp"

class QModelIndex;

namespace CSMDoc
{
    class Document;
}

namespace CSVRender
{
    class SceneWidget;
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
            CSVRender::SceneWidget *mScene;
            CSVRender::Navigation1st m1st;
            CSVRender::NavigationFree mFree;
            CSVRender::NavigationOrbit mOrbit;

        public:

            SceneSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document);

            virtual void setEditLock (bool locked);

            virtual void updateEditorSetting (const QString& key, const QString& value);

            virtual void setStatusBar (bool show);

        private slots:

            void selectNavigationMode (const std::string& mode);
    };
}

#endif
