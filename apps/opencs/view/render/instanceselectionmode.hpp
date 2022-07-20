#ifndef CSV_RENDER_INSTANCE_SELECTION_MODE_H
#define CSV_RENDER_INSTANCE_SELECTION_MODE_H

#include <QPoint>

#include <osg/PositionAttitudeTransform>
#include <osg/Vec3d>

#include "selectionmode.hpp"
#include "instancedragmodes.hpp"

namespace CSVRender
{
    class InstanceSelectionMode : public SelectionMode
    {
            Q_OBJECT

        public:

            InstanceSelectionMode(CSVWidget::SceneToolbar* parent, WorldspaceWidget& worldspaceWidget, osg::Group *cellNode);

            ~InstanceSelectionMode();

            /// Store the worldspace-coordinate when drag begins
            void setDragStart(const osg::Vec3d& dragStart);

            /// Store the worldspace-coordinate when drag begins
            const osg::Vec3d& getDragStart();

            /// Store the screen-coordinate when drag begins
            void setScreenDragStart(const QPoint& dragStartPoint);

            /// Apply instance selection changes
            void dragEnded(const osg::Vec3d& dragEndPoint, DragMode dragMode);

            void drawSelectionCubeCentre(const osg::Vec3f& mousePlanePoint );
            void drawSelectionCubeCorner(const osg::Vec3f& mousePlanePoint );
            void drawSelectionSphere(const osg::Vec3f& mousePlanePoint );
        protected:

            /// Add context menu items to \a menu.
            ///
            /// \attention menu can be a 0-pointer
            ///
            /// \return Have there been any menu items to be added (if menu is 0 and there
            /// items to be added, the function must return true anyway.
            bool createContextMenu(QMenu* menu) override;

        private:

            void drawSelectionBox(const osg::Vec3d& pointA, const osg::Vec3d& pointB);
            void drawSelectionCube(const osg::Vec3d& point, float radius);
            void drawSelectionSphere(const osg::Vec3d& point, float radius);

            QAction* mDeleteSelection;
            QAction* mSelectSame;
            osg::Vec3d mDragStart;
            osg::Group* mParentNode;
            osg::ref_ptr<osg::PositionAttitudeTransform> mBaseNode;

        private slots:

            void deleteSelection();
            void selectSame();
    };
}

#endif
