#ifndef OPENCS_VIEW_WORLDSPACEWIDGET_H
#define OPENCS_VIEW_WORLDSPACEWIDGET_H

#include "scenewidget.hpp"

#include "navigation1st.hpp"
#include "navigationfree.hpp"
#include "navigationorbit.hpp"
#include <apps/opencs/model/doc/document.hpp>
#include <apps/opencs/model/world/tablemimedata.hpp>

namespace CSMWorld
{
    class UniversalId;
}
namespace CSVWidget
{
    class SceneToolMode;
    class SceneToolToggle;
    class SceneToolbar;
}

namespace CSVRender
{
    class WorldspaceWidget : public SceneWidget
    {
            Q_OBJECT

            CSVRender::Navigation1st m1st;
            CSVRender::NavigationFree mFree;
            CSVRender::NavigationOrbit mOrbit;
            CSVWidget::SceneToolToggle *mSceneElements;

        public:

            enum dropType
            {
                cellsMixed,
                cellsInterior,
                cellsExterior,
                notCells
            };

            enum dropRequirments
            {
                canHandle,
                needPaged,
                needUnpaged,
                ignored //either mixed cells, or not cells
            };

            WorldspaceWidget (CSMDoc::Document& document, QWidget *parent = 0);

            CSVWidget::SceneToolMode *makeNavigationSelector (CSVWidget::SceneToolbar *parent);
            ///< \attention The created tool is not added to the toolbar (via addTool). Doing that
            /// is the responsibility of the calling function.

            /// \attention The created tool is not added to the toolbar (via addTool). Doing
            /// that is the responsibility of the calling function.
            CSVWidget::SceneToolToggle *makeSceneVisibilitySelector (
                CSVWidget::SceneToolbar *parent);

            void selectDefaultNavigationMode();

            static dropType getDropType(const std::vector<CSMWorld::UniversalId>& data);

            virtual dropRequirments getDropRequirements(dropType type) const = 0;

            virtual void useViewHint (const std::string& hint);
            ///< Default-implementation: ignored.

            virtual void handleDrop(const std::vector<CSMWorld::UniversalId>& data) = 0;

            virtual unsigned int getElementMask() const;

        protected:

            virtual void addVisibilitySelectorButtons (CSVWidget::SceneToolToggle *tool);

            const CSMDoc::Document& mDocument;

        private:

            void dragEnterEvent(QDragEnterEvent *event);

            void dropEvent(QDropEvent* event);

            void dragMoveEvent(QDragMoveEvent *event);

        private slots:

            void selectNavigationMode (const std::string& mode);

            virtual void referenceableDataChanged (const QModelIndex& topLeft,
                const QModelIndex& bottomRight) = 0;

            virtual void referenceableAboutToBeRemoved (const QModelIndex& parent, int start, int end) = 0;

            virtual void referenceableAdded (const QModelIndex& index, int start, int end) = 0;

            virtual void referenceDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight) = 0;

            virtual void referenceAboutToBeRemoved (const QModelIndex& parent, int start, int end) = 0;

            virtual void referenceAdded (const QModelIndex& index, int start, int end) = 0;

        protected slots:

            void elementSelectionChanged();

        signals:

            void closeRequest();
            void dataDropped(const std::vector<CSMWorld::UniversalId>& data);
    };
}

#endif