#ifndef CSV_WORLD_SCENESUBVIEW_H
#define CSV_WORLD_SCENESUBVIEW_H

#include <QHBoxLayout>

#include "../doc/subview.hpp"

class QModelIndex;

namespace CSMWorld
{
    class CellSelection;
}

namespace CSMDoc
{
    class Document;
}

namespace CSVRender
{
    class WorldspaceWidget;
    class PagedWorldspaceWidget;
    class UnpagedWorldspaceWidget;
}

namespace CSVWidget
{
    class SceneToolbar;
    class SceneToolMode;
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
            QHBoxLayout* mLayout;
            CSMDoc::Document& mDocument;
            CSVWidget::SceneToolbar* mToolbar;
            std::string mTitle;

        public:

            SceneSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document);

            virtual void setEditLock (bool locked);

            virtual void setStatusBar (bool show);

            virtual void useHint (const std::string& hint);

            virtual std::string getTitle() const;

        private:

            void makeConnections(CSVRender::PagedWorldspaceWidget* widget);

            void makeConnections(CSVRender::UnpagedWorldspaceWidget* widget);

            void replaceToolbarAndWorldspace(CSVRender::WorldspaceWidget* widget, CSVWidget::SceneToolbar* toolbar);

            enum widgetType
            {
                widget_Paged,
                widget_Unpaged
            };

            CSVWidget::SceneToolbar* makeToolbar(CSVRender::WorldspaceWidget* widget, widgetType type);

        private slots:

            void cellSelectionChanged (const CSMWorld::CellSelection& selection);

            void cellSelectionChanged (const CSMWorld::UniversalId& id);

            void handleDrop(const std::vector<CSMWorld::UniversalId>& data);

        signals:

            void requestFocus (const std::string& id);
    };
}

#endif
