#ifndef OPENCS_VIEW_WORLDSPACEWIDGET_H
#define OPENCS_VIEW_WORLDSPACEWIDGET_H

#include "scenewidget.hpp"
#include "mousestate.hpp"

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
    class SceneToolRun;
}

namespace CSVWorld
{
    class PhysicsSystem;
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
            CSVWidget::SceneToolRun *mRun;
            CSMDoc::Document& mDocument;
            CSVWorld::PhysicsSystem *mPhysics;
            MouseState *mMouse;
            unsigned int mInteractionMask;

        public:

            enum DropType
            {
                Type_CellsInterior,
                Type_CellsExterior,
                Type_Other,
                Type_DebugProfile
            };

            enum dropRequirments
            {
                canHandle,
                needPaged,
                needUnpaged,
                ignored //either mixed cells, or not cells
            };

            WorldspaceWidget (CSMDoc::Document& document, QWidget *parent = 0);
            ~WorldspaceWidget ();

            CSVWidget::SceneToolMode *makeNavigationSelector (CSVWidget::SceneToolbar *parent);
            ///< \attention The created tool is not added to the toolbar (via addTool). Doing that
            /// is the responsibility of the calling function.

            /// \attention The created tool is not added to the toolbar (via addTool). Doing
            /// that is the responsibility of the calling function.
            CSVWidget::SceneToolToggle *makeSceneVisibilitySelector (
                CSVWidget::SceneToolbar *parent);

            /// \attention The created tool is not added to the toolbar (via addTool). Doing
            /// that is the responsibility of the calling function.
            CSVWidget::SceneToolRun *makeRunTool (CSVWidget::SceneToolbar *parent);

            /// \attention The created tool is not added to the toolbar (via addTool). Doing
            /// that is the responsibility of the calling function.
            CSVWidget::SceneToolMode *makeEditModeSelector (CSVWidget::SceneToolbar *parent);

            void selectDefaultNavigationMode();

            static DropType getDropType(const std::vector<CSMWorld::UniversalId>& data);

            virtual dropRequirments getDropRequirements(DropType type) const;

            virtual void useViewHint (const std::string& hint);
            ///< Default-implementation: ignored.

            /// \return Drop handled?
            virtual bool handleDrop (const std::vector<CSMWorld::UniversalId>& data,
                DropType type);

            virtual unsigned int getVisibilityMask() const;

            /// \note This function will implicitly add elements that are independent of the
            /// selected edit mode.
            virtual void setInteractionMask (unsigned int mask);

            /// \note This function will only return those elements that are both visible and
            /// marked for interaction.
            unsigned int getInteractionMask() const;

        protected:

            virtual void addVisibilitySelectorButtons (CSVWidget::SceneToolToggle *tool);

            virtual void addEditModeSelectorButtons (CSVWidget::SceneToolMode *tool);

            CSMDoc::Document& getDocument();

            virtual void updateOverlay();

            CSVWorld::PhysicsSystem *getPhysics();

            virtual void mouseMoveEvent (QMouseEvent *event);
            virtual void mousePressEvent (QMouseEvent *event);
            virtual void mouseReleaseEvent (QMouseEvent *event);
            virtual void mouseDoubleClickEvent (QMouseEvent *event);
            virtual void wheelEvent (QWheelEvent *event);
            virtual void keyPressEvent (QKeyEvent *event);

            // FIXME: temporary only until the signals from the document are implemented
            virtual void pathgridInserted (const std::string &terrain, const Ogre::Vector3 &pos);
            virtual void pathgridMoved (const std::string &pgName, const Ogre::Vector3 &pos);
            virtual void pathgridAboutToBeRemoved (const std::string &pgName);

        private:

            void dragEnterEvent(QDragEnterEvent *event);

            void dropEvent(QDropEvent* event);

            void dragMoveEvent(QDragMoveEvent *event);

            virtual std::string getStartupInstruction() = 0;

        private slots:

            void selectNavigationMode (const std::string& mode);

            virtual void referenceableDataChanged (const QModelIndex& topLeft,
                const QModelIndex& bottomRight) = 0;

            virtual void referenceableAboutToBeRemoved (const QModelIndex& parent, int start, int end) = 0;

            virtual void referenceableAdded (const QModelIndex& index, int start, int end) = 0;

            virtual void referenceDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight) = 0;

            virtual void referenceAboutToBeRemoved (const QModelIndex& parent, int start, int end) = 0;

            virtual void referenceAdded (const QModelIndex& index, int start, int end) = 0;

            virtual void runRequest (const std::string& profile);

            void debugProfileDataChanged (const QModelIndex& topLeft,
                const QModelIndex& bottomRight);

            void debugProfileAboutToBeRemoved (const QModelIndex& parent, int start, int end);

            //virtual void pathgridAdded (const QModelIndex& index, int start, int end) = 0;

            //virtual void pathgridDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight) = 0;

            //virtual void pathgridAboutToBeRemoved (const QModelIndex& parent, int start, int end) = 0;

        protected slots:

            void elementSelectionChanged();

        signals:

            void closeRequest();

            void dataDropped(const std::vector<CSMWorld::UniversalId>& data);

        friend class MouseState;
    };
}

#endif
