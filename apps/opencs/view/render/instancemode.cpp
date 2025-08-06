#include "instancemode.hpp"

#include <QDragEnterEvent>
#include <QPoint>
#include <QString>

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <memory>
#include <utility>

#include "../../model/prefs/state.hpp"

#include <osg/BoundingBox>
#include <osg/Camera>
#include <osg/ComputeBoundsVisitor>
#include <osg/Group>
#include <osg/Math>
#include <osg/Matrix>
#include <osg/Matrixd>
#include <osg/Vec3d>
#include <osg/Viewport>
#include <osgUtil/IntersectionVisitor>
#include <osgUtil/LineSegmentIntersector>

#include <apps/opencs/model/doc/document.hpp>
#include <apps/opencs/model/prefs/category.hpp>
#include <apps/opencs/model/prefs/setting.hpp>
#include <apps/opencs/model/world/cellcoordinates.hpp>
#include <apps/opencs/model/world/cellselection.hpp>
#include <apps/opencs/model/world/columns.hpp>
#include <apps/opencs/model/world/data.hpp>
#include <apps/opencs/model/world/idcollection.hpp>
#include <apps/opencs/model/world/refcollection.hpp>
#include <apps/opencs/model/world/universalid.hpp>
#include <apps/opencs/view/render/editmode.hpp>
#include <apps/opencs/view/render/instancedragmodes.hpp>
#include <apps/opencs/view/render/tagbase.hpp>

#include <components/esm/defs.hpp>
#include <components/misc/scalableicon.hpp>

#include "../../model/prefs/shortcut.hpp"
#include "../../model/world/commandmacro.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/idtree.hpp"
#include "../../model/world/tablemimedata.hpp"

#include "../widget/scenetoolbar.hpp"
#include "../widget/scenetoolmode.hpp"

#include "mask.hpp"

#include "instancemovemode.hpp"
#include "instanceselectionmode.hpp"
#include "object.hpp"
#include "pagedworldspacewidget.hpp"
#include "worldspacewidget.hpp"

int CSVRender::InstanceMode::getSubModeFromId(const std::string& id) const
{
    return id == "move" ? 0 : (id == "rotate" ? 1 : 2);
}

osg::Vec3f CSVRender::InstanceMode::quatToEuler(const osg::Quat& rot) const
{
    float x, y, z;
    float test = 2 * (rot.w() * rot.y() + rot.x() * rot.z());

    if (std::abs(test) >= 1.f)
    {
        x = atan2(rot.x(), rot.w());
        y = (test > 0) ? (osg::PI / 2) : (-osg::PI / 2);
        z = 0;
    }
    else
    {
        x = std::atan2(2 * (rot.w() * rot.x() - rot.y() * rot.z()), 1 - 2 * (rot.x() * rot.x() + rot.y() * rot.y()));
        y = std::asin(test);
        z = std::atan2(2 * (rot.w() * rot.z() - rot.x() * rot.y()), 1 - 2 * (rot.y() * rot.y() + rot.z() * rot.z()));
    }

    return osg::Vec3f(-x, -y, -z);
}

osg::Quat CSVRender::InstanceMode::eulerToQuat(const osg::Vec3f& euler) const
{
    osg::Quat xr = osg::Quat(-euler[0], osg::Vec3f(1, 0, 0));
    osg::Quat yr = osg::Quat(-euler[1], osg::Vec3f(0, 1, 0));
    osg::Quat zr = osg::Quat(-euler[2], osg::Vec3f(0, 0, 1));

    return zr * yr * xr;
}

float CSVRender::InstanceMode::roundFloatToMult(const float val, const double mult) const
{
    if (mult == 0)
        return val;
    return round(val / mult) * mult;
}

osg::Vec3 CSVRender::InstanceMode::calculateSnapPositionRelativeToTarget(osg::Vec3 initalPosition,
    osg::Vec3 targetPosition, osg::Vec3 targetRotation, osg::Vec3 translation, double snap) const
{
    auto quatTargetRotation
        = osg::Quat(targetRotation[0], osg::X_AXIS, targetRotation[1], osg::Y_AXIS, targetRotation[2], osg::Z_AXIS);

    // Break object world coords into snap target space
    auto localWorld = osg::Matrix::translate(initalPosition)
        * osg::Matrix::inverse(osg::Matrix::translate(targetPosition)) * osg::Matrix::rotate(quatTargetRotation);

    osg::Vec3 localPosition = localWorld.getTrans();

    osg::Vec3 newTranslation;
    newTranslation[0] = CSVRender::InstanceMode::roundFloatToMult(localPosition[0] + translation[0], snap);
    newTranslation[1] = CSVRender::InstanceMode::roundFloatToMult(localPosition[1] + translation[1], snap);
    newTranslation[2] = CSVRender::InstanceMode::roundFloatToMult(localPosition[2] + translation[2], snap);

    // rebuild object's world coordinates (note: inverse operations from local construction)
    auto newObjectWorld = osg::Matrix::translate(newTranslation)
        * osg::Matrix::inverse(osg::Matrix::rotate(quatTargetRotation)) * osg::Matrix::translate(targetPosition);

    osg::Vec3 newObjectPosition = newObjectWorld.getTrans();

    return newObjectPosition;
}

osg::Vec3f CSVRender::InstanceMode::getSelectionCenter(const std::vector<osg::ref_ptr<TagBase>>& selection) const
{
    osg::Vec3f center = osg::Vec3f(0, 0, 0);
    int objectCount = 0;

    for (std::vector<osg::ref_ptr<TagBase>>::const_iterator iter(selection.begin()); iter != selection.end(); ++iter)
    {
        if (CSVRender::ObjectTag* objectTag = dynamic_cast<CSVRender::ObjectTag*>(iter->get()))
        {
            const ESM::Position& position = objectTag->mObject->getPosition();
            center += osg::Vec3f(position.pos[0], position.pos[1], position.pos[2]);

            ++objectCount;
        }
    }

    if (objectCount > 0)
        center /= objectCount;

    return center;
}

osg::Vec3f CSVRender::InstanceMode::getScreenCoords(const osg::Vec3f& pos)
{
    osg::Matrix viewMatrix = getWorldspaceWidget().getCamera()->getViewMatrix();
    osg::Matrix projMatrix = getWorldspaceWidget().getCamera()->getProjectionMatrix();
    osg::Matrix windowMatrix = getWorldspaceWidget().getCamera()->getViewport()->computeWindowMatrix();
    osg::Matrix combined = viewMatrix * projMatrix * windowMatrix;

    return pos * combined;
}

osg::Vec3f CSVRender::InstanceMode::getProjectionSpaceCoords(const osg::Vec3f& pos)
{
    osg::Matrix viewMatrix = getWorldspaceWidget().getCamera()->getViewMatrix();
    osg::Matrix projMatrix = getWorldspaceWidget().getCamera()->getProjectionMatrix();
    osg::Matrix combined = viewMatrix * projMatrix;

    return pos * combined;
}

osg::Vec3f CSVRender::InstanceMode::getMousePlaneCoords(const QPoint& point, const osg::Vec3d& dragStart)
{
    const osg::Matrix viewMatrix = getWorldspaceWidget().getCamera()->getViewMatrix();
    const osg::Matrix projMatrix = getWorldspaceWidget().getCamera()->getProjectionMatrix();
    const osg::Matrix combined = osg::Matrix::inverse(viewMatrix * projMatrix);

    /* calculate viewport normalized coordinates
       note: is there a reason to use getCamera()->getViewport()->computeWindowMatrix() instead? */
    const float scale = getWorldspaceWidget().devicePixelRatioF();
    const osg::Viewport* viewport = getWorldspaceWidget().getCamera()->getViewport();
    float x = point.x() * scale / viewport->width();
    float y = point.y() * scale / viewport->height();
    x = x * 2.0f - 1.0f;
    y = 1.0f - y * 2.0f;

    osg::Vec3f mousePlanePoint = osg::Vec3f(x, y, dragStart.z()) * combined;

    return mousePlanePoint;
}

void CSVRender::InstanceMode::saveSelectionGroup(const int group)
{
    QStringList strings;
    QUndoStack& undoStack = getWorldspaceWidget().getDocument().getUndoStack();
    QVariant selectionObjects;
    CSMWorld::CommandMacro macro(undoStack, "Replace Selection Group");
    std::string groupName = "project::" + std::to_string(group);

    const auto& selection = getWorldspaceWidget().getSelection(Mask_Reference);
    const int selectionObjectsIndex
        = mSelectionGroups->findColumnIndex(CSMWorld::Columns::ColumnId_SelectionGroupObjects);

    if (dynamic_cast<CSVRender::PagedWorldspaceWidget*>(&getWorldspaceWidget()))
        groupName += "-ext";
    else
        groupName += "-" + getWorldspaceWidget().getCellId(osg::Vec3f(0, 0, 0));

    CSMWorld::CreateCommand* newGroup = new CSMWorld::CreateCommand(*mSelectionGroups, groupName);

    newGroup->setType(CSMWorld::UniversalId::Type_SelectionGroup);

    for (const auto& object : selection)
        if (const CSVRender::ObjectTag* objectTag = dynamic_cast<CSVRender::ObjectTag*>(object.get()))
            strings << QString::fromStdString(objectTag->mObject->getReferenceId());

    selectionObjects.setValue(strings);

    newGroup->addValue(selectionObjectsIndex, selectionObjects);

    if (mSelectionGroups->getModelIndex(groupName, 0).row() != -1)
        macro.push(new CSMWorld::DeleteCommand(*mSelectionGroups, groupName));

    macro.push(newGroup);

    getWorldspaceWidget().clearSelection(Mask_Reference);
}

void CSVRender::InstanceMode::getSelectionGroup(const int group)
{
    std::string groupName = "project::" + std::to_string(group);
    std::vector<std::string> targets;

    const auto& selection = getWorldspaceWidget().getSelection(Mask_Reference);
    const int selectionObjectsIndex
        = mSelectionGroups->findColumnIndex(CSMWorld::Columns::ColumnId_SelectionGroupObjects);

    if (dynamic_cast<CSVRender::PagedWorldspaceWidget*>(&getWorldspaceWidget()))
        groupName += "-ext";
    else
        groupName += "-" + getWorldspaceWidget().getCellId(osg::Vec3f(0, 0, 0));

    const QModelIndex groupSearch = mSelectionGroups->getModelIndex(groupName, selectionObjectsIndex);

    if (groupSearch.row() == -1)
        return;

    for (const QString& target : groupSearch.data().toStringList())
        targets.push_back(target.toStdString());

    if (!selection.empty())
        getWorldspaceWidget().clearSelection(Mask_Reference);

    getWorldspaceWidget().selectGroup(targets);
}

void CSVRender::InstanceMode::setDragAxis(const char axis)
{
    int newDragAxis;

    const std::vector<osg::ref_ptr<TagBase>> selection = getWorldspaceWidget().getSelection(Mask_Reference);

    if (selection.empty())
        return;

    switch (axis)
    {
        case 'x':
            newDragAxis = 0;
            break;
        case 'y':
            newDragAxis = 1;
            break;
        case 'z':
            newDragAxis = 2;
            break;
        default:
            return;
    }

    if (newDragAxis == mDragAxis)
        newDragAxis = -1;

    if (mSubModeId == "move")
    {
        mObjectsAtDragStart.clear();

        for (const auto& object : selection)
            if (CSVRender::ObjectTag* objectTag = dynamic_cast<CSVRender::ObjectTag*>(object.get()))
            {
                const osg::Vec3f thisPoint = objectTag->mObject->getPosition().asVec3();
                mDragStart = thisPoint;
                mObjectsAtDragStart.emplace_back(thisPoint);
            }
    }
    mDragAxis = newDragAxis;
}

QString CSVRender::InstanceMode::getTooltip()
{
    return QString(
        "Instance editing"
        "<ul><li>Use {scene-select-primary} and {scene-select-secondary} to select and unselect instances</li>"
        "<li>Use {scene-edit-primary} to manipulate instances</li>"
        "<li>Use {scene-select-tertiary} to select a reference object and then {scene-edit-secondary} to snap "
        "selection relative to the reference object</li>"
        "<li>Use {scene-submode-move}, {scene-submode-rotate}, {scene-submode-scale} to change to move, "
        "rotate, and "
        "scale modes respectively</li>"
        "<li>Use {scene-axis-x}, {scene-axis-y}, and {scene-axis-z} to lock changes to X, Y, and Z axes "
        "respectively</li>"
        "<li>Use {scene-delete} to delete currently selected objects</li>"
        "<li>Use {scene-duplicate} to duplicate instances</li>"
        "<li>Use {scene-instance-drop} to drop instances</li></ul>");
}

CSVRender::InstanceMode::InstanceMode(
    WorldspaceWidget* worldspaceWidget, osg::ref_ptr<osg::Group> parentNode, QWidget* parent)
    : EditMode(worldspaceWidget, Misc::ScalableIcon::load(":scenetoolbar/editing-instance"),
        Mask_Reference | Mask_Terrain, getTooltip(), parent)
    , mSubMode(nullptr)
    , mSubModeId("move")
    , mSelectionMode(nullptr)
    , mDragMode(DragMode_None)
    , mDragAxis(-1)
    , mLocked(false)
    , mUnitScaleDist(1)
    , mParentNode(std::move(parentNode))
{
    mSelectionGroups = dynamic_cast<CSMWorld::IdTable*>(
        worldspaceWidget->getDocument().getData().getTableModel(CSMWorld::UniversalId::Type_SelectionGroup));

    connect(this, &InstanceMode::requestFocus, worldspaceWidget, &WorldspaceWidget::requestFocus);

    CSMPrefs::Shortcut* deleteShortcut = new CSMPrefs::Shortcut("scene-delete", worldspaceWidget);
    connect(deleteShortcut, qOverload<>(&CSMPrefs::Shortcut::activated), this, &InstanceMode::deleteSelectedInstances);

    CSMPrefs::Shortcut* duplicateShortcut = new CSMPrefs::Shortcut("scene-duplicate", worldspaceWidget);

    connect(
        duplicateShortcut, qOverload<>(&CSMPrefs::Shortcut::activated), this, &InstanceMode::cloneSelectedInstances);

    connect(new CSMPrefs::Shortcut("scene-instance-drop", worldspaceWidget),
        qOverload<>(&CSMPrefs::Shortcut::activated), this, &InstanceMode::dropToCollision);

    for (short i = 0; i <= 9; i++)
    {
        connect(new CSMPrefs::Shortcut("scene-group-" + std::to_string(i), worldspaceWidget),
            qOverload<>(&CSMPrefs::Shortcut::activated), this, [this, i] { this->getSelectionGroup(i); });
        connect(new CSMPrefs::Shortcut("scene-save-" + std::to_string(i), worldspaceWidget),
            qOverload<>(&CSMPrefs::Shortcut::activated), this, [this, i] { this->saveSelectionGroup(i); });
    }

    connect(new CSMPrefs::Shortcut("scene-submode-move", worldspaceWidget), qOverload<>(&CSMPrefs::Shortcut::activated),
        this, [this] { mSubMode->setButton("move"); });

    connect(new CSMPrefs::Shortcut("scene-submode-scale", worldspaceWidget),
        qOverload<>(&CSMPrefs::Shortcut::activated), this, [this] { mSubMode->setButton("scale"); });

    connect(new CSMPrefs::Shortcut("scene-submode-rotate", worldspaceWidget),
        qOverload<>(&CSMPrefs::Shortcut::activated), this, [this] { mSubMode->setButton("rotate"); });

    for (const char axis : "xyz")
        connect(new CSMPrefs::Shortcut(std::string("scene-axis-") + axis, worldspaceWidget),
            qOverload<>(&CSMPrefs::Shortcut::activated), this, [this, axis] {
                this->setDragAxis(axis);
                std::string axisStr(1, toupper(axis));
                switch (getSubMode())
                {
                    case (Object::Mode_Move):
                        axisStr += "_Axis";
                        break;
                    case (Object::Mode_Rotate):
                        axisStr += "_Axis_Rot";
                        break;
                    case (Object::Mode_Scale):
                        axisStr += "_Axis_Scale";
                        break;
                }

                auto selectionMarker = getWorldspaceWidget().getSelectionMarker();

                if (mDragAxis != -1)
                    selectionMarker->updateMarkerHighlight(axisStr, axis - 'x');
                else
                    selectionMarker->resetMarkerHighlight();
            });
}

void CSVRender::InstanceMode::activate(CSVWidget::SceneToolbar* toolbar)
{
    if (!mSubMode)
    {
        mSubMode = new CSVWidget::SceneToolMode(toolbar, "Edit Sub-Mode");
        mSubMode->addButton(new InstanceMoveMode(this), "move");
        mSubMode->addButton(":scenetoolbar/transform-rotate", "rotate",
            "Rotate selected instances"
            "<ul><li>Use {scene-edit-primary} to rotate instances freely</li>"
            "<li>Use {scene-edit-secondary} to rotate instances within the grid</li>"
            "<li>The center of the view acts as the axis of rotation</li>"
            "</ul>");
        mSubMode->addButton(":scenetoolbar/transform-scale", "scale",
            "Scale selected instances"
            "<ul><li>Use {scene-edit-primary} to scale instances freely</li>"
            "<li>Use {scene-edit-secondary} to scale instances along the grid</li>"
            "<li>The scaling rate is based on how close the start of a drag is to the center of the screen</li>"
            "</ul>");

        mSubMode->setButton(mSubModeId);

        connect(mSubMode, &CSVWidget::SceneToolMode::modeChanged, this, &InstanceMode::subModeChanged);
    }

    if (!mSelectionMode)
        mSelectionMode = new InstanceSelectionMode(toolbar, getWorldspaceWidget(), mParentNode);

    mDragMode = DragMode_None;

    EditMode::activate(toolbar);

    toolbar->addTool(mSubMode);
    toolbar->addTool(mSelectionMode);

    std::string subMode = mSubMode->getCurrentId();

    getWorldspaceWidget().setSubMode(getSubModeFromId(subMode), Mask_Reference);
}

void CSVRender::InstanceMode::deactivate(CSVWidget::SceneToolbar* toolbar)
{
    mDragMode = DragMode_None;
    getWorldspaceWidget().reset(Mask_Reference);

    if (mSelectionMode)
    {
        toolbar->removeTool(mSelectionMode);
        delete mSelectionMode;
        mSelectionMode = nullptr;
    }

    if (mSubMode)
    {
        toolbar->removeTool(mSubMode);
        delete mSubMode;
        mSubMode = nullptr;
    }

    EditMode::deactivate(toolbar);
}

void CSVRender::InstanceMode::setEditLock(bool locked)
{
    mLocked = locked;

    if (mLocked)
        getWorldspaceWidget().abortDrag();
}

void CSVRender::InstanceMode::primaryEditPressed(const WorldspaceHitResult& hit)
{
    if (CSMPrefs::get()["3D Scene Input"]["context-select"].isTrue())
        primarySelectPressed(hit);
}

void CSVRender::InstanceMode::primaryOpenPressed(const WorldspaceHitResult& hit)
{
    if (hit.tag)
    {
        if (CSVRender::ObjectTag* objectTag = dynamic_cast<CSVRender::ObjectTag*>(hit.tag.get()))
        {
            const std::string refId = objectTag->mObject->getReferenceId();
            emit requestFocus(refId);
        }
    }
}

void CSVRender::InstanceMode::secondaryEditPressed(const WorldspaceHitResult& hit)
{
    if (CSMPrefs::get()["3D Scene Input"]["context-select"].isTrue())
        secondarySelectPressed(hit);
}

void CSVRender::InstanceMode::primarySelectPressed(const WorldspaceHitResult& hit)
{
    auto& worldspaceWidget = getWorldspaceWidget();

    worldspaceWidget.clearSelection(Mask_Reference);

    if (!hit.tag)
        return;

    if (CSVRender::ObjectTag* objectTag = dynamic_cast<CSVRender::ObjectTag*>(hit.tag.get()))
    {
        // hit an Object, select it
        CSVRender::Object* object = objectTag->mObject;
        object->setSelected(true);
        worldspaceWidget.getSelectionMarker()->addToSelectionHistory(object->getReferenceId());
    }
}

void CSVRender::InstanceMode::secondarySelectPressed(const WorldspaceHitResult& hit)
{
    if (!hit.tag)
        return;

    if (CSVRender::ObjectTag* objectTag = dynamic_cast<CSVRender::ObjectTag*>(hit.tag.get()))
    {
        // hit an Object, toggle its selection state
        CSVRender::Object* object = objectTag->mObject;
        object->setSelected(!object->getSelected());

        const auto selectionMarker = getWorldspaceWidget().getSelectionMarker();

        if (object->getSelected())
            selectionMarker->addToSelectionHistory(object->getReferenceId(), false);

        selectionMarker->updateSelectionMarker();
    }
}

void CSVRender::InstanceMode::tertiarySelectPressed(const WorldspaceHitResult& hit)
{
    if (auto* snapTarget
        = dynamic_cast<CSVRender::ObjectTag*>(getWorldspaceWidget().getSnapTarget(Mask_Reference).get()))
    {
        snapTarget->mObject->setSnapTarget(false);
    }

    if (!hit.tag)
        return;

    if (CSVRender::ObjectTag* objectTag = dynamic_cast<CSVRender::ObjectTag*>(hit.tag.get()))
    {
        // hit an Object, toggle its selection state
        CSVRender::Object* object = objectTag->mObject;
        object->setSnapTarget(!object->getSnapTarget());
    }
}

bool CSVRender::InstanceMode::primaryEditStartDrag(const QPoint& pos)
{
    if (mDragMode != DragMode_None || mLocked)
        return false;

    auto& worldspaceWidget = getWorldspaceWidget();

    WorldspaceHitResult hit = worldspaceWidget.mousePick(pos, worldspaceWidget.getInteractionMask());

    std::vector<osg::ref_ptr<TagBase>> selection = worldspaceWidget.getSelection(Mask_Reference);
    if (selection.empty())
    {
        // Only change selection at the start of drag if no object is already selected
        if (hit.tag && CSMPrefs::get()["3D Scene Input"]["context-select"].isTrue())
        {
            worldspaceWidget.clearSelection(Mask_Reference);
            if (CSVRender::ObjectTag* objectTag = dynamic_cast<CSVRender::ObjectTag*>(hit.tag.get()))
            {
                CSVRender::Object* object = objectTag->mObject;
                object->setSelected(true);
                worldspaceWidget.getSelectionMarker()->addToSelectionHistory(object->getReferenceId());
            }
        }

        selection = worldspaceWidget.getSelection(Mask_Reference);
        if (selection.empty())
            return false;
    }

    mObjectsAtDragStart.clear();

    for (std::vector<osg::ref_ptr<TagBase>>::iterator iter(selection.begin()); iter != selection.end(); ++iter)
    {
        if (CSVRender::ObjectTag* objectTag = dynamic_cast<CSVRender::ObjectTag*>(iter->get()))
        {
            if (mSubModeId == "move")
            {
                objectTag->mObject->setEdited(Object::Override_Position);
                float x = objectTag->mObject->getPosition().pos[0];
                float y = objectTag->mObject->getPosition().pos[1];
                float z = objectTag->mObject->getPosition().pos[2];
                osg::Vec3f thisPoint(x, y, z);
                mDragStart = getMousePlaneCoords(pos, getProjectionSpaceCoords(thisPoint));
                mObjectsAtDragStart.emplace_back(thisPoint);
                mDragMode = DragMode_Move;
            }
            else if (mSubModeId == "rotate")
            {
                objectTag->mObject->setEdited(Object::Override_Rotation);
                mDragMode = DragMode_Rotate;
            }
            else if (mSubModeId == "scale")
            {
                objectTag->mObject->setEdited(Object::Override_Scale);
                mDragMode = DragMode_Scale;

                // Calculate scale factor
                std::vector<osg::ref_ptr<TagBase>> editedSelection = getWorldspaceWidget().getEdited(Mask_Reference);
                osg::Vec3f center = getScreenCoords(getSelectionCenter(editedSelection));

                int widgetHeight = getWorldspaceWidget().height();

                float dx = pos.x() - center.x();
                float dy = (widgetHeight - pos.y()) - center.y();

                mUnitScaleDist = std::sqrt(dx * dx + dy * dy);
            }
        }
    }

    if (CSVRender::ObjectMarkerTag* objectTag = dynamic_cast<CSVRender::ObjectMarkerTag*>(hit.tag.get()))
    {
        mDragAxis = objectTag->mAxis;
    }
    else
        mDragAxis = -1;

    return true;
}

bool CSVRender::InstanceMode::secondaryEditStartDrag(const QPoint& pos)
{
    if (mDragMode != DragMode_None || mLocked)
        return false;

    auto& worldspaceWidget = getWorldspaceWidget();

    WorldspaceHitResult hit = worldspaceWidget.mousePick(pos, worldspaceWidget.getInteractionMask());

    std::vector<osg::ref_ptr<TagBase>> selection = worldspaceWidget.getSelection(Mask_Reference);
    if (selection.empty())
    {
        // Only change selection at the start of drag if no object is already selected
        if (hit.tag && CSMPrefs::get()["3D Scene Input"]["context-select"].isTrue())
        {
            worldspaceWidget.clearSelection(Mask_Reference);
            if (CSVRender::ObjectTag* objectTag = dynamic_cast<CSVRender::ObjectTag*>(hit.tag.get()))
            {
                CSVRender::Object* object = objectTag->mObject;
                object->setSelected(true);
                worldspaceWidget.getSelectionMarker()->addToSelectionHistory(object->getReferenceId());
            }
        }

        selection = worldspaceWidget.getSelection(Mask_Reference);
        if (selection.empty())
            return false;
    }

    mObjectsAtDragStart.clear();

    for (std::vector<osg::ref_ptr<TagBase>>::iterator iter(selection.begin()); iter != selection.end(); ++iter)
    {
        if (CSVRender::ObjectTag* objectTag = dynamic_cast<CSVRender::ObjectTag*>(iter->get()))
        {
            if (mSubModeId == "move")
            {
                objectTag->mObject->setEdited(Object::Override_Position);
                float x = objectTag->mObject->getPosition().pos[0];
                float y = objectTag->mObject->getPosition().pos[1];
                float z = objectTag->mObject->getPosition().pos[2];
                osg::Vec3f thisPoint(x, y, z);

                mDragStart = getMousePlaneCoords(pos, getProjectionSpaceCoords(thisPoint));
                mObjectsAtDragStart.emplace_back(thisPoint);
                mDragMode = DragMode_Move_Snap;
            }
            else if (mSubModeId == "rotate")
            {
                objectTag->mObject->setEdited(Object::Override_Rotation);
                mDragMode = DragMode_Rotate_Snap;
            }
            else if (mSubModeId == "scale")
            {
                objectTag->mObject->setEdited(Object::Override_Scale);
                mDragMode = DragMode_Scale_Snap;

                // Calculate scale factor
                std::vector<osg::ref_ptr<TagBase>> editedSelection = worldspaceWidget.getEdited(Mask_Reference);
                osg::Vec3f center = getScreenCoords(getSelectionCenter(editedSelection));

                int widgetHeight = worldspaceWidget.height();

                float dx = pos.x() - center.x();
                float dy = (widgetHeight - pos.y()) - center.y();

                mUnitScaleDist = std::sqrt(dx * dx + dy * dy);
            }
        }
    }

    if (CSVRender::ObjectMarkerTag* objectTag = dynamic_cast<CSVRender::ObjectMarkerTag*>(hit.tag.get()))
    {
        mDragAxis = objectTag->mAxis;
    }
    else
        mDragAxis = -1;

    return true;
}

bool CSVRender::InstanceMode::primarySelectStartDrag(const QPoint& pos)
{
    if (mDragMode != DragMode_None || mLocked)
        return false;

    std::string primarySelectAction = CSMPrefs::get()["3D Scene Editing"]["primary-select-action"].toString();

    if (primarySelectAction == "Select only")
        mDragMode = DragMode_Select_Only;
    else if (primarySelectAction == "Add to selection")
        mDragMode = DragMode_Select_Add;
    else if (primarySelectAction == "Remove from selection")
        mDragMode = DragMode_Select_Remove;
    else if (primarySelectAction == "Invert selection")
        mDragMode = DragMode_Select_Invert;

    WorldspaceHitResult hit = getWorldspaceWidget().mousePick(pos, getWorldspaceWidget().getInteractionMask());
    mSelectionMode->setDragStart(hit.worldPos);

    return true;
}

bool CSVRender::InstanceMode::secondarySelectStartDrag(const QPoint& pos)
{
    if (mDragMode != DragMode_None || mLocked)
        return false;

    std::string secondarySelectAction = CSMPrefs::get()["3D Scene Editing"]["secondary-select-action"].toString();

    if (secondarySelectAction == "Select only")
        mDragMode = DragMode_Select_Only;
    else if (secondarySelectAction == "Add to selection")
        mDragMode = DragMode_Select_Add;
    else if (secondarySelectAction == "Remove from selection")
        mDragMode = DragMode_Select_Remove;
    else if (secondarySelectAction == "Invert selection")
        mDragMode = DragMode_Select_Invert;

    WorldspaceHitResult hit = getWorldspaceWidget().mousePick(pos, getWorldspaceWidget().getInteractionMask());
    mSelectionMode->setDragStart(hit.worldPos);

    return true;
}

void CSVRender::InstanceMode::drag(const QPoint& pos, int diffX, int diffY, double speedFactor)
{
    osg::Vec3f offset;
    osg::Quat rotation;

    std::vector<osg::ref_ptr<TagBase>> selection = getWorldspaceWidget().getEdited(Mask_Reference);
    auto* snapTarget = dynamic_cast<CSVRender::ObjectTag*>(getWorldspaceWidget().getSnapTarget(Mask_Reference).get());

    if (mDragMode == DragMode_Move || mDragMode == DragMode_Move_Snap)
    {
    }
    else if (mDragMode == DragMode_Rotate || mDragMode == DragMode_Rotate_Snap)
    {
        osg::Vec3f eye, centre, up;
        getWorldspaceWidget().getCamera()->getViewMatrix().getLookAt(eye, centre, up);

        float angle;
        osg::Vec3f axis;

        if (mDragAxis == -1)
        {
            // Free rotate
            float rotationFactor = CSMPrefs::get()["3D Scene Input"]["rotate-factor"].toDouble() * speedFactor;

            osg::Quat cameraRotation = getWorldspaceWidget().getCamera()->getInverseViewMatrix().getRotate();

            osg::Vec3f camForward = centre - eye;
            osg::Vec3f screenDir = cameraRotation * osg::Vec3f(diffX, diffY, 0);
            screenDir.normalize();

            angle = std::sqrt(diffX * diffX + diffY * diffY) * rotationFactor;
            axis = screenDir ^ camForward;
        }
        else
        {
            // Global axis rotation
            osg::Vec3f camBack = eye - centre;

            for (int i = 0; i < 3; ++i)
            {
                if (i == mDragAxis)
                    axis[i] = 1;
                else
                    axis[i] = 0;
            }

            // Flip axis if facing opposite side
            if (camBack * axis < 0)
                axis *= -1;

            // Convert coordinate system
            osg::Vec3f screenCenter = getScreenCoords(getSelectionCenter(selection));

            int widgetHeight = getWorldspaceWidget().height();

            float newX = pos.x() - screenCenter.x();
            float newY = (widgetHeight - pos.y()) - screenCenter.y();

            float oldX = newX - diffX;
            float oldY = newY - diffY; // diffY appears to already be flipped

            osg::Vec3f oldVec = osg::Vec3f(oldX, oldY, 0);
            oldVec.normalize();

            osg::Vec3f newVec = osg::Vec3f(newX, newY, 0);
            newVec.normalize();

            // Find angle and axis of rotation
            angle = std::acos(oldVec * newVec) * speedFactor;
            if (((oldVec ^ newVec) * camBack < 0) ^ (camBack.z() < 0))
                angle *= -1;
        }

        rotation = osg::Quat(angle, axis);
    }
    else if (mDragMode == DragMode_Scale || mDragMode == DragMode_Scale_Snap)
    {
        osg::Vec3f center = getScreenCoords(getSelectionCenter(selection));

        // Calculate scaling distance/rate
        int widgetHeight = getWorldspaceWidget().height();

        float dx = pos.x() - center.x();
        float dy = (widgetHeight - pos.y()) - center.y();

        float dist = std::sqrt(dx * dx + dy * dy);
        float scale = dist / mUnitScaleDist;

        // Only uniform scaling is currently supported
        offset = osg::Vec3f(scale, scale, scale);
    }
    else if (mSelectionMode->getCurrentId() == "cube-centre")
    {
        osg::Vec3f mousePlanePoint = getMousePlaneCoords(pos, getProjectionSpaceCoords(mSelectionMode->getDragStart()));
        mSelectionMode->drawSelectionCubeCentre(mousePlanePoint);
        return;
    }
    else if (mSelectionMode->getCurrentId() == "cube-corner")
    {
        osg::Vec3f mousePlanePoint = getMousePlaneCoords(pos, getProjectionSpaceCoords(mSelectionMode->getDragStart()));
        mSelectionMode->drawSelectionCubeCorner(mousePlanePoint);
        return;
    }
    else if (mSelectionMode->getCurrentId() == "sphere")
    {
        osg::Vec3f mousePlanePoint = getMousePlaneCoords(pos, getProjectionSpaceCoords(mSelectionMode->getDragStart()));
        mSelectionMode->drawSelectionSphere(mousePlanePoint);
        return;
    }

    int i = 0;

    // Apply
    for (std::vector<osg::ref_ptr<TagBase>>::iterator iter(selection.begin()); iter != selection.end(); ++iter, i++)
    {
        if (CSVRender::ObjectTag* objectTag = dynamic_cast<CSVRender::ObjectTag*>(iter->get()))
        {
            if (mDragMode == DragMode_Move || mDragMode == DragMode_Move_Snap)
            {
                ESM::Position position = objectTag->mObject->getPosition();
                osg::Vec3f mousePos = getMousePlaneCoords(pos, getProjectionSpaceCoords(mDragStart));
                float addToX = mousePos.x() - mDragStart.x();
                float addToY = mousePos.y() - mDragStart.y();
                float addToZ = mousePos.z() - mDragStart.z();
                position.pos[0] = mObjectsAtDragStart[i].x() + addToX;
                position.pos[1] = mObjectsAtDragStart[i].y() + addToY;
                position.pos[2] = mObjectsAtDragStart[i].z() + addToZ;

                if (mDragMode == DragMode_Move_Snap)
                {
                    double snap = CSMPrefs::get()["3D Scene Editing"]["gridsnap-movement"].toDouble();

                    if (snapTarget)
                    {
                        osg::Vec3 translation(addToX, addToY, addToZ);

                        auto snapTargetPosition = snapTarget->mObject->getPosition();
                        auto newPosition = calculateSnapPositionRelativeToTarget(mObjectsAtDragStart[i],
                            snapTargetPosition.asVec3(), snapTargetPosition.asRotationVec3(), translation, snap);

                        position.pos[0] = newPosition[0];
                        position.pos[1] = newPosition[1];
                        position.pos[2] = newPosition[2];
                    }
                    else
                    {
                        position.pos[0] = CSVRender::InstanceMode::roundFloatToMult(position.pos[0], snap);
                        position.pos[1] = CSVRender::InstanceMode::roundFloatToMult(position.pos[1], snap);
                        position.pos[2] = CSVRender::InstanceMode::roundFloatToMult(position.pos[2], snap);
                    }
                }

                // XYZ-locking
                if (mDragAxis != -1)
                {
                    for (int j = 0; j < 3; ++j)
                    {
                        if (j != mDragAxis)
                            position.pos[j] = mObjectsAtDragStart[i][j];
                    }
                }

                objectTag->mObject->setPosition(position.pos);
            }
            else if (mDragMode == DragMode_Rotate || mDragMode == DragMode_Rotate_Snap)
            {
                ESM::Position position = objectTag->mObject->getPosition();

                osg::Quat currentRot = eulerToQuat(osg::Vec3f(position.rot[0], position.rot[1], position.rot[2]));
                osg::Quat combined = currentRot * rotation;

                osg::Vec3f euler = quatToEuler(combined);
                // There appears to be a very rare rounding error that can cause asin to return NaN
                if (!euler.isNaN())
                {
                    position.rot[0] = euler.x();
                    position.rot[1] = euler.y();
                    position.rot[2] = euler.z();
                }

                objectTag->mObject->setRotation(position.rot);
            }
            else if (mDragMode == DragMode_Scale || mDragMode == DragMode_Scale_Snap)
            {
                // Reset scale
                objectTag->mObject->setEdited(0);
                objectTag->mObject->setEdited(Object::Override_Scale);

                float scale = objectTag->mObject->getScale();
                scale *= offset.x();

                if (mDragMode == DragMode_Scale_Snap)
                {
                    scale = CSVRender::InstanceMode::roundFloatToMult(
                        scale, CSMPrefs::get()["3D Scene Editing"]["gridsnap-scale"].toDouble());
                }

                objectTag->mObject->setScale(scale);
            }
        }
    }
}

void CSVRender::InstanceMode::dragCompleted(const QPoint& pos)
{
    std::vector<osg::ref_ptr<TagBase>> selection = getWorldspaceWidget().getEdited(Mask_Reference);

    auto* snapTarget = dynamic_cast<CSVRender::ObjectTag*>(getWorldspaceWidget().getSnapTarget(Mask_Reference).get());

    QUndoStack& undoStack = getWorldspaceWidget().getDocument().getUndoStack();

    QString description;

    switch (mDragMode)
    {
        case DragMode_Move:
            description = "Move Instances";
            break;
        case DragMode_Rotate:
            description = "Rotate Instances";
            break;
        case DragMode_Scale:
            description = "Scale Instances";
            break;
        case DragMode_Select_Only:
            handleSelectDrag(pos);
            return;
            break;
        case DragMode_Select_Add:
            handleSelectDrag(pos);
            return;
            break;
        case DragMode_Select_Remove:
            handleSelectDrag(pos);
            return;
            break;
        case DragMode_Select_Invert:
            handleSelectDrag(pos);
            return;
            break;
        case DragMode_Move_Snap:
            description = "Move Instances";
            break;
        case DragMode_Rotate_Snap:
            description = "Rotate Instances";
            break;
        case DragMode_Scale_Snap:
            description = "Scale Instances";
            break;
        case DragMode_None:
            break;
    }

    CSMWorld::CommandMacro macro(undoStack, description);

    // Is this even supposed to be here?
    for (std::vector<osg::ref_ptr<TagBase>>::iterator iter(selection.begin()); iter != selection.end(); ++iter)
    {
        if (CSVRender::ObjectTag* objectTag = dynamic_cast<CSVRender::ObjectTag*>(iter->get()))
        {
            if (mDragMode == DragMode_Rotate_Snap)
            {
                ESM::Position position = objectTag->mObject->getPosition();
                double snap = CSMPrefs::get()["3D Scene Editing"]["gridsnap-rotation"].toDouble();

                float xOffset = 0;
                float yOffset = 0;
                float zOffset = 0;

                if (snapTarget)
                {
                    auto snapTargetPosition = snapTarget->mObject->getPosition();
                    auto rotation = snapTargetPosition.rot;
                    if (rotation)
                    {
                        xOffset = remainder(rotation[0], osg::DegreesToRadians(snap));
                        yOffset = remainder(rotation[1], osg::DegreesToRadians(snap));
                        zOffset = remainder(rotation[2], osg::DegreesToRadians(snap));
                    }
                }

                position.rot[0]
                    = CSVRender::InstanceMode::roundFloatToMult(position.rot[0], osg::DegreesToRadians(snap)) + xOffset;
                position.rot[1]
                    = CSVRender::InstanceMode::roundFloatToMult(position.rot[1], osg::DegreesToRadians(snap)) + yOffset;
                position.rot[2]
                    = CSVRender::InstanceMode::roundFloatToMult(position.rot[2], osg::DegreesToRadians(snap)) + zOffset;

                objectTag->mObject->setRotation(position.rot);
            }

            objectTag->mObject->apply(macro);
        }
    }

    mObjectsAtDragStart.clear();
    mDragMode = DragMode_None;
}

void CSVRender::InstanceMode::dragAborted()
{
    getWorldspaceWidget().reset(Mask_Reference);
    mDragMode = DragMode_None;
}

void CSVRender::InstanceMode::dragWheel(int diff, double speedFactor)
{
    if (mDragMode == DragMode_Move || mDragMode == DragMode_Move_Snap)
    {
        osg::Vec3f eye;
        osg::Vec3f centre;
        osg::Vec3f up;

        getWorldspaceWidget().getCamera()->getViewMatrix().getLookAt(eye, centre, up);

        osg::Vec3f offset = centre - eye;
        offset.normalize();
        offset *= diff * speedFactor;

        std::vector<osg::ref_ptr<TagBase>> selection = getWorldspaceWidget().getEdited(Mask_Reference);
        auto snapTarget
            = dynamic_cast<CSVRender::ObjectTag*>(getWorldspaceWidget().getSnapTarget(Mask_Reference).get());

        int j = 0;

        for (std::vector<osg::ref_ptr<TagBase>>::iterator iter(selection.begin()); iter != selection.end(); ++iter, j++)
        {
            if (CSVRender::ObjectTag* objectTag = dynamic_cast<CSVRender::ObjectTag*>(iter->get()))
            {
                ESM::Position position = objectTag->mObject->getPosition();
                auto preMovedObjectPosition = position.asVec3();
                for (int i = 0; i < 3; ++i)
                    position.pos[i] += offset[i];

                if (mDragMode == DragMode_Move_Snap)
                {
                    double snap = CSMPrefs::get()["3D Scene Editing"]["gridsnap-movement"].toDouble();

                    if (snapTarget)
                    {
                        osg::Vec3 translation(snap, snap, snap);

                        auto snapTargetPosition = snapTarget->mObject->getPosition();
                        auto newPosition = calculateSnapPositionRelativeToTarget(preMovedObjectPosition,
                            snapTargetPosition.asVec3(), snapTargetPosition.asRotationVec3(), translation, snap);

                        position.pos[0] = newPosition[0];
                        position.pos[1] = newPosition[1];
                        position.pos[2] = newPosition[2];
                    }
                    else
                    {
                        position.pos[0] = CSVRender::InstanceMode::roundFloatToMult(position.pos[0], snap);
                        position.pos[1] = CSVRender::InstanceMode::roundFloatToMult(position.pos[1], snap);
                        position.pos[2] = CSVRender::InstanceMode::roundFloatToMult(position.pos[2], snap);
                    }
                }

                objectTag->mObject->setPosition(position.pos);
                osg::Vec3f thisPoint(position.pos[0], position.pos[1], position.pos[2]);
                mDragStart = getMousePlaneCoords(
                    getWorldspaceWidget().mapFromGlobal(QCursor::pos()), getProjectionSpaceCoords(thisPoint));
                mObjectsAtDragStart[j] = thisPoint;
            }
        }
    }
}

void CSVRender::InstanceMode::dragEnterEvent(QDragEnterEvent* event)
{
    if (const CSMWorld::TableMimeData* mime = dynamic_cast<const CSMWorld::TableMimeData*>(event->mimeData()))
    {
        if (!mime->fromDocument(getWorldspaceWidget().getDocument()))
            return;

        if (mime->holdsType(CSMWorld::UniversalId::Type_Referenceable))
            event->accept();
    }
}

void CSVRender::InstanceMode::dropEvent(QDropEvent* event)
{
    if (const CSMWorld::TableMimeData* mime = dynamic_cast<const CSMWorld::TableMimeData*>(event->mimeData()))
    {
        CSMDoc::Document& document = getWorldspaceWidget().getDocument();

        if (!mime->fromDocument(document))
            return;

        WorldspaceHitResult hit
            = getWorldspaceWidget().mousePick(event->position().toPoint(), getWorldspaceWidget().getInteractionMask());

        std::string cellId = getWorldspaceWidget().getCellId(hit.worldPos);

        CSMWorld::IdTree& cellTable
            = dynamic_cast<CSMWorld::IdTree&>(*document.getData().getTableModel(CSMWorld::UniversalId::Type_Cells));

        const bool noCell = document.getData().getCells().searchId(ESM::RefId::stringRefId(cellId)) == -1;

        if (noCell)
        {
            std::string mode = CSMPrefs::get()["3D Scene Editing"]["outside-drop"].toString();

            // target cell does not exist
            if (mode == "Discard")
                return;

            if (mode == "Create cell and insert")
            {
                std::unique_ptr<CSMWorld::CreateCommand> createCommand(new CSMWorld::CreateCommand(cellTable, cellId));

                int parentIndex = cellTable.findColumnIndex(CSMWorld::Columns::ColumnId_Cell);
                int index = cellTable.findNestedColumnIndex(parentIndex, CSMWorld::Columns::ColumnId_Interior);
                createCommand->addNestedValue(parentIndex, index, false);

                document.getUndoStack().push(createCommand.release());

                if (CSVRender::PagedWorldspaceWidget* paged
                    = dynamic_cast<CSVRender::PagedWorldspaceWidget*>(&getWorldspaceWidget()))
                {
                    CSMWorld::CellSelection selection = paged->getCellSelection();
                    selection.add(CSMWorld::CellCoordinates::fromId(cellId).first);
                    paged->setCellSelection(selection);
                }
            }
        }
        else if (CSVRender::PagedWorldspaceWidget* paged
            = dynamic_cast<CSVRender::PagedWorldspaceWidget*>(&getWorldspaceWidget()))
        {
            CSMWorld::CellSelection selection = paged->getCellSelection();
            if (!selection.has(CSMWorld::CellCoordinates::fromId(cellId).first))
            {
                // target cell exists, but is not shown
                std::string mode = CSMPrefs::get()["3D Scene Editing"]["outside-visible-drop"].toString();

                if (mode == "Discard")
                    return;

                if (mode == "Show cell and insert")
                {
                    selection.add(CSMWorld::CellCoordinates::fromId(cellId).first);
                    paged->setCellSelection(selection);
                }
            }
        }

        CSMWorld::IdTable& referencesTable = dynamic_cast<CSMWorld::IdTable&>(
            *document.getData().getTableModel(CSMWorld::UniversalId::Type_References));

        bool dropped = false;

        std::vector<CSMWorld::UniversalId> ids = mime->getData();

        for (std::vector<CSMWorld::UniversalId>::const_iterator iter(ids.begin()); iter != ids.end(); ++iter)
            if (mime->isReferencable(iter->getType()))
            {
                // create reference
                std::unique_ptr<CSMWorld::CreateCommand> createCommand(
                    new CSMWorld::CreateCommand(referencesTable, document.getData().getReferences().getNewId()));

                createCommand->addValue(referencesTable.findColumnIndex(CSMWorld::Columns::ColumnId_Cell),
                    QString::fromUtf8(cellId.c_str()));
                createCommand->addValue(
                    referencesTable.findColumnIndex(CSMWorld::Columns::ColumnId_PositionXPos), hit.worldPos.x());
                createCommand->addValue(
                    referencesTable.findColumnIndex(CSMWorld::Columns::ColumnId_PositionYPos), hit.worldPos.y());
                createCommand->addValue(
                    referencesTable.findColumnIndex(CSMWorld::Columns::ColumnId_PositionZPos), hit.worldPos.z());
                createCommand->addValue(referencesTable.findColumnIndex(CSMWorld::Columns::ColumnId_ReferenceableId),
                    QString::fromUtf8(iter->getId().c_str()));

                document.getUndoStack().push(createCommand.release());

                dropped = true;
            }

        if (dropped)
            event->accept();
    }
}

int CSVRender::InstanceMode::getSubMode() const
{
    return mSubMode ? getSubModeFromId(mSubMode->getCurrentId()) : 0;
}

void CSVRender::InstanceMode::subModeChanged(const std::string& id)
{
    mSubModeId = id;
    getWorldspaceWidget().abortDrag();
    getWorldspaceWidget().setSubMode(getSubModeFromId(id), Mask_Reference);
}

void CSVRender::InstanceMode::handleSelectDrag(const QPoint& pos)
{
    osg::Vec3f mousePlanePoint = getMousePlaneCoords(pos, getProjectionSpaceCoords(mSelectionMode->getDragStart()));
    mSelectionMode->dragEnded(mousePlanePoint, mDragMode);
    mDragMode = DragMode_None;
}

void CSVRender::InstanceMode::deleteSelectedInstances()
{
    std::vector<osg::ref_ptr<TagBase>> selection = getWorldspaceWidget().getSelection(Mask_Reference);
    if (selection.empty())
        return;

    CSMDoc::Document& document = getWorldspaceWidget().getDocument();
    CSMWorld::IdTable& referencesTable
        = dynamic_cast<CSMWorld::IdTable&>(*document.getData().getTableModel(CSMWorld::UniversalId::Type_References));
    QUndoStack& undoStack = document.getUndoStack();

    CSMWorld::CommandMacro macro(undoStack, "Delete Instances");
    for (osg::ref_ptr<TagBase> tag : selection)
        if (CSVRender::ObjectTag* objectTag = dynamic_cast<CSVRender::ObjectTag*>(tag.get()))
            macro.push(new CSMWorld::DeleteCommand(referencesTable, objectTag->mObject->getReferenceId()));

    getWorldspaceWidget().clearSelection(Mask_Reference);
}

void CSVRender::InstanceMode::cloneSelectedInstances()
{
    std::vector<osg::ref_ptr<TagBase>> selection = getWorldspaceWidget().getSelection(Mask_Reference);
    if (selection.empty())
        return;

    CSMDoc::Document& document = getWorldspaceWidget().getDocument();
    CSMWorld::IdTable& referencesTable
        = dynamic_cast<CSMWorld::IdTable&>(*document.getData().getTableModel(CSMWorld::UniversalId::Type_References));
    QUndoStack& undoStack = document.getUndoStack();

    CSMWorld::CommandMacro macro(undoStack, "Clone Instances");
    for (osg::ref_ptr<TagBase> tag : selection)
        if (CSVRender::ObjectTag* objectTag = dynamic_cast<CSVRender::ObjectTag*>(tag.get()))
        {
            macro.push(new CSMWorld::CloneCommand(referencesTable, objectTag->mObject->getReferenceId(),
                document.getData().getReferences().getNewId(), CSMWorld::UniversalId::Type_Reference));
        }
}

void CSVRender::InstanceMode::dropInstance(CSVRender::Object* object, float dropHeight)
{
    object->setEdited(Object::Override_Position);
    ESM::Position position = object->getPosition();
    position.pos[2] -= dropHeight;
    object->setPosition(position.pos);
}

float CSVRender::InstanceMode::calculateDropHeight(CSVRender::Object* object, float objectHeight)
{
    osg::Vec3d point = object->getPosition().asVec3();

    osg::Vec3d start = point;
    start.z() += objectHeight;
    osg::Vec3d end = point;
    end.z() = std::numeric_limits<float>::lowest();

    osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector(
        new osgUtil::LineSegmentIntersector(osgUtil::Intersector::MODEL, start, end));
    intersector->setIntersectionLimit(osgUtil::LineSegmentIntersector::NO_LIMIT);
    osgUtil::IntersectionVisitor visitor(intersector);

    visitor.setTraversalMask(Mask_Terrain | Mask_Reference);

    mParentNode->accept(visitor);

    osgUtil::LineSegmentIntersector::Intersections::iterator it = intersector->getIntersections().begin();
    if (it != intersector->getIntersections().end())
    {
        osgUtil::LineSegmentIntersector::Intersection intersection = *it;
        float collisionLevel = intersection.getWorldIntersectPoint().z();
        return point.z() - collisionLevel + objectHeight;
    }

    return 0.0f;
}

void CSVRender::InstanceMode::dropToCollision()
{
    std::vector<osg::ref_ptr<TagBase>> selection = getWorldspaceWidget().getSelection(Mask_Reference);
    if (selection.empty())
        return;

    CSMDoc::Document& document = getWorldspaceWidget().getDocument();
    QUndoStack& undoStack = document.getUndoStack();

    CSMWorld::CommandMacro macro(undoStack, "Drop objects to collision");

    DropObjectHeightHandler dropObjectDataHandler(&getWorldspaceWidget());

    int counter = 0;
    for (osg::ref_ptr<TagBase> tag : selection)
        if (CSVRender::ObjectTag* objectTag = dynamic_cast<CSVRender::ObjectTag*>(tag.get()))
        {
            float objectHeight = dropObjectDataHandler.mObjectHeights[counter++];
            float dropHeight = calculateDropHeight(objectTag->mObject, objectHeight);
            dropInstance(objectTag->mObject, dropHeight);
            objectTag->mObject->apply(macro);
        }
}

CSVRender::DropObjectHeightHandler::DropObjectHeightHandler(WorldspaceWidget* worldspacewidget)
    : mWorldspaceWidget(worldspacewidget)
{
    std::vector<osg::ref_ptr<TagBase>> selection = mWorldspaceWidget->getSelection(Mask_Reference);
    for (osg::ref_ptr<TagBase> tag : selection)
    {
        if (CSVRender::ObjectTag* objectTag = dynamic_cast<CSVRender::ObjectTag*>(tag.get()))
        {
            osg::ref_ptr<osg::Group> objectNodeWithGUI = objectTag->mObject->getRootNode();
            osg::ref_ptr<osg::Group> objectNodeWithoutGUI = objectTag->mObject->getBaseNode();

            osg::ComputeBoundsVisitor computeBounds;
            computeBounds.setTraversalMask(Mask_Reference);
            objectNodeWithoutGUI->accept(computeBounds);
            osg::BoundingBox bounds = computeBounds.getBoundingBox();
            float boundingBoxOffset = 0.0f;
            if (bounds.valid())
                boundingBoxOffset = bounds.zMin();

            mObjectHeights.emplace_back(boundingBoxOffset);
            mOldMasks.emplace_back(objectNodeWithGUI->getNodeMask());

            objectNodeWithGUI->setNodeMask(0);
        }
    }
}

CSVRender::DropObjectHeightHandler::~DropObjectHeightHandler()
{
    std::vector<osg::ref_ptr<TagBase>> selection = mWorldspaceWidget->getSelection(Mask_Reference);
    int counter = 0;
    for (osg::ref_ptr<TagBase> tag : selection)
    {
        if (CSVRender::ObjectTag* objectTag = dynamic_cast<CSVRender::ObjectTag*>(tag.get()))
        {
            osg::ref_ptr<osg::Group> objectNodeWithGUI = objectTag->mObject->getRootNode();
            objectNodeWithGUI->setNodeMask(mOldMasks[counter]);
            counter++;
        }
    }
}
