#ifndef CSV_RENDER_INSTANCEMODE_H
#define CSV_RENDER_INSTANCEMODE_H

#include <QString>

#include <osg/ref_ptr>
#include <osg/Group>
#include <osg/Quat>
#include <osg/Vec3f>

#include "editmode.hpp"

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

            enum DragMode
            {
                DragMode_None,
                DragMode_Move,
                DragMode_Rotate,
                DragMode_Scale
            };

            enum DropMode
            {
                Collision,
                Terrain,
                CollisionSep,
                TerrainSep
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
            void dropInstance(DropMode dropMode, CSVRender::Object* object, float objectHeight);
            float getDropHeight(DropMode dropMode, CSVRender::Object* object, float objectHeight);

        public:

            InstanceMode (WorldspaceWidget *worldspaceWidget, osg::ref_ptr<osg::Group> parentNode, QWidget *parent = 0);

            virtual void activate (CSVWidget::SceneToolbar *toolbar);

            virtual void deactivate (CSVWidget::SceneToolbar *toolbar);

            virtual void setEditLock (bool locked);

            virtual void primaryOpenPressed (const WorldspaceHitResult& hit);

            virtual void primaryEditPressed (const WorldspaceHitResult& hit);

            virtual void secondaryEditPressed (const WorldspaceHitResult& hit);

            virtual void primarySelectPressed (const WorldspaceHitResult& hit);

            virtual void secondarySelectPressed (const WorldspaceHitResult& hit);

            virtual bool primaryEditStartDrag (const QPoint& pos);

            virtual bool secondaryEditStartDrag (const QPoint& pos);

            virtual void drag (const QPoint& pos, int diffX, int diffY, double speedFactor);

            virtual void dragCompleted(const QPoint& pos);

            /// \note dragAborted will not be called, if the drag is aborted via changing
            /// editing mode
            virtual void dragAborted();

            virtual void dragWheel (int diff, double speedFactor);

            virtual void dragEnterEvent (QDragEnterEvent *event);

            virtual void dropEvent (QDropEvent *event);

            virtual int getSubMode() const;

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
    class DropObjectDataHandler
    {
        public:
            DropObjectDataHandler(WorldspaceWidget* worldspacewidget);
            ~DropObjectDataHandler();
            std::vector<float> mObjectHeights;

        private:
            WorldspaceWidget* mWorldspaceWidget;
            std::vector<osg::Node::NodeMask> mOldMasks;
    };
}

#endif
