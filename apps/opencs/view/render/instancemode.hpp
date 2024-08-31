#ifndef CSV_RENDER_INSTANCEMODE_H
#define CSV_RENDER_INSTANCEMODE_H

#include <QString>

#include <string>
#include <vector>

#include <osg/Group>
#include <osg/Node>
#include <osg/Quat>
#include <osg/Vec3>
#include <osg/ref_ptr>

#include "editmode.hpp"
#include "instancedragmodes.hpp"
#include <apps/opencs/model/world/idtable.hpp>
#include <components/esm3/selectiongroup.hpp>

class QDragEnterEvent;
class QDropEvent;
class QObject;
class QPoint;
class QWidget;

namespace CSVWidget
{
    class SceneToolMode;
    class SceneToolbar;
}

namespace CSVRender
{
    class TagBase;
    class InstanceSelectionMode;
    class Object;
    class WorldspaceWidget;
    struct WorldspaceHitResult;

    class InstanceMode : public EditMode
    {
        Q_OBJECT

        CSVWidget::SceneToolMode* mSubMode;
        std::string mSubModeId;
        InstanceSelectionMode* mSelectionMode;
        DragMode mDragMode;
        int mDragAxis;
        bool mLocked;
        float mUnitScaleDist;
        osg::ref_ptr<osg::Group> mParentNode;
        osg::Vec3 mDragStart;
        std::vector<osg::Vec3> mObjectsAtDragStart;
        CSMWorld::IdTable* mSelectionGroups;

        int getSubModeFromId(const std::string& id) const;

        osg::Vec3 quatToEuler(const osg::Quat& quat) const;
        osg::Quat eulerToQuat(const osg::Vec3& euler) const;

        float roundFloatToMult(const float val, const double mult) const;

        osg::Vec3 getSelectionCenter(const std::vector<osg::ref_ptr<TagBase>>& selection) const;
        osg::Vec3 getScreenCoords(const osg::Vec3& pos);
        osg::Vec3 getProjectionSpaceCoords(const osg::Vec3& pos);
        osg::Vec3 getMousePlaneCoords(const QPoint& point, const osg::Vec3d& dragStart);
        void handleSelectDrag(const QPoint& pos);
        void dropInstance(CSVRender::Object* object, float dropHeight);
        float calculateDropHeight(CSVRender::Object* object, float objectHeight);
        osg::Vec3 calculateSnapPositionRelativeToTarget(osg::Vec3 initalPosition, osg::Vec3 targetPosition,
            osg::Vec3 targetRotation, osg::Vec3 translation, double snap) const;

    public:
        InstanceMode(
            WorldspaceWidget* worldspaceWidget, osg::ref_ptr<osg::Group> parentNode, QWidget* parent = nullptr);

        QString getTooltip();

        void activate(CSVWidget::SceneToolbar* toolbar) override;

        void deactivate(CSVWidget::SceneToolbar* toolbar) override;

        void setEditLock(bool locked) override;

        void primaryOpenPressed(const WorldspaceHitResult& hit) override;

        void primaryEditPressed(const WorldspaceHitResult& hit) override;

        void secondaryEditPressed(const WorldspaceHitResult& hit) override;

        void primarySelectPressed(const WorldspaceHitResult& hit) override;

        void secondarySelectPressed(const WorldspaceHitResult& hit) override;

        void tertiarySelectPressed(const WorldspaceHitResult& hit) override;

        bool primaryEditStartDrag(const QPoint& pos) override;

        bool secondaryEditStartDrag(const QPoint& pos) override;

        bool primarySelectStartDrag(const QPoint& pos) override;

        bool secondarySelectStartDrag(const QPoint& pos) override;

        void drag(const QPoint& pos, int diffX, int diffY, double speedFactor) override;

        void dragCompleted(const QPoint& pos) override;

        /// \note dragAborted will not be called, if the drag is aborted via changing
        /// editing mode
        void dragAborted() override;

        void dragWheel(int diff, double speedFactor) override;

        void dragEnterEvent(QDragEnterEvent* event) override;

        void dropEvent(QDropEvent* event) override;

        int getSubMode() const override;

    signals:

        void requestFocus(const std::string& id);

    private slots:

        void setDragAxis(const char axis);
        void subModeChanged(const std::string& id);
        void deleteSelectedInstances();
        void cloneSelectedInstances();
        void getSelectionGroup(const int group);
        void saveSelectionGroup(const int group);
        void dropToCollision();
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
