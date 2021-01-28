#ifndef CSV_RENDER_INSTANCEMODE_H
#define CSV_RENDER_INSTANCEMODE_H

#include <QString>

#include <osg/ref_ptr>
#include <osg/Group>
#include <osg/Quat>
#include <osg/Vec3f>

#include "editmode.hpp"
#include "instancedragmodes.hpp"

namespace CSVWidget
{
    class SceneToolMode;
}

namespace CSVRender
{
    class TagBase;
    class InstanceSelectionMode;
    class Object;

    class InstanceMode : public EditMode
    {
            Q_OBJECT

            enum DropMode
            {
                Separate = 0b1,

                Collision = 0b10,
                Terrain = 0b100,

                CollisionSep = Collision | Separate,
                TerrainSep = Terrain | Separate,
            };

            CSVWidget::SceneToolMode *mSubMode;
            std::string mSubModeId;
            InstanceSelectionMode *mSelectionMode;
            DragMode mDragMode;
            int mDragAxis;
            bool mLocked;
            float mUnitScaleDist;
            osg::ref_ptr<osg::Group> mParentNode;

            int getSubModeFromId (const std::string& id) const;

            osg::Vec3f quatToEuler(const osg::Quat& quat) const;
            osg::Quat eulerToQuat(const osg::Vec3f& euler) const;

            osg::Vec3f getSelectionCenter(const std::vector<osg::ref_ptr<TagBase> >& selection) const;
            osg::Vec3f getScreenCoords(const osg::Vec3f& pos);
            osg::Vec3f getProjectionSpaceCoords(const osg::Vec3f& pos);
            osg::Vec3f getMousePlaneCoords(const QPoint& point, const osg::Vec3d& dragStart);
            void handleSelectDrag(const QPoint& pos);
            void dropInstance(CSVRender::Object* object, float dropHeight);
            float calculateDropHeight(DropMode dropMode, CSVRender::Object* object, float objectHeight);

        public:

            InstanceMode (WorldspaceWidget *worldspaceWidget, osg::ref_ptr<osg::Group> parentNode, QWidget *parent = nullptr);

            void activate (CSVWidget::SceneToolbar *toolbar) override;

            void deactivate (CSVWidget::SceneToolbar *toolbar) override;

            void setEditLock (bool locked) override;

            void primaryOpenPressed (const WorldspaceHitResult& hit) override;

            void primaryEditPressed (const WorldspaceHitResult& hit) override;

            void secondaryEditPressed (const WorldspaceHitResult& hit) override;

            void primarySelectPressed (const WorldspaceHitResult& hit) override;

            void secondarySelectPressed (const WorldspaceHitResult& hit) override;

            bool primaryEditStartDrag (const QPoint& pos) override;

            bool secondaryEditStartDrag (const QPoint& pos) override;

            bool primarySelectStartDrag(const QPoint& pos) override;

            bool secondarySelectStartDrag(const QPoint& pos) override;

            void drag (const QPoint& pos, int diffX, int diffY, double speedFactor) override;

            void dragCompleted(const QPoint& pos) override;

            /// \note dragAborted will not be called, if the drag is aborted via changing
            /// editing mode
            void dragAborted() override;

            void dragWheel (int diff, double speedFactor) override;

            void dragEnterEvent (QDragEnterEvent *event) override;

            void dropEvent (QDropEvent *event) override;

            int getSubMode() const override;

        signals:

            void requestFocus (const std::string& id);

        private slots:

            void subModeChanged (const std::string& id);
            void deleteSelectedInstances(bool active);
            void dropSelectedInstancesToCollision();
            void dropSelectedInstancesToTerrain();
            void dropSelectedInstancesToCollisionSeparately();
            void dropSelectedInstancesToTerrainSeparately();
            void handleDropMethod(DropMode dropMode, QString commandMsg);
    };

    /// \brief Helper class to handle object mask data in safe way
    class DropObjectHeightHandler
    {
        public:
            DropObjectHeightHandler(WorldspaceWidget* worldspacewidget);
            ~DropObjectHeightHandler();
            std::vector<float> mObjectHeights;

        private:
            WorldspaceWidget* mWorldspaceWidget;
            std::vector<osg::Node::NodeMask> mOldMasks;
    };
}

#endif
