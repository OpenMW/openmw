#include "worldspacewidget.hpp"

#include <algorithm>
#include <set>

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QScreen>
#include <QToolTip>
#include <QWindow>

#include <apps/opencs/model/doc/document.hpp>
#include <apps/opencs/model/prefs/category.hpp>
#include <apps/opencs/model/prefs/setting.hpp>
#include <apps/opencs/model/world/columns.hpp>
#include <apps/opencs/model/world/data.hpp>
#include <apps/opencs/model/world/record.hpp>
#include <apps/opencs/view/render/editmode.hpp>
#include <apps/opencs/view/render/scenewidget.hpp>
#include <apps/opencs/view/widget/modebutton.hpp>

#include <components/esm/defs.hpp>
#include <components/misc/scalableicon.hpp>

#include <osg/Camera>
#include <osg/Group>
#include <osg/Matrixd>
#include <osg/Node>
#include <osg/Referenced>
#include <osg/Viewport>
#include <osgUtil/IntersectionVisitor>
#include <osgViewer/View>

#include <osgUtil/LineSegmentIntersector>

#include "../../model/world/idtable.hpp"
#include "../../model/world/tablemimedata.hpp"
#include "../../model/world/universalid.hpp"

#include "../../model/prefs/shortcut.hpp"
#include "../../model/prefs/state.hpp"

#include "../render/orbitcameramode.hpp"

#include "../widget/scenetoolmode.hpp"
#include "../widget/scenetoolrun.hpp"
#include "../widget/scenetooltoggle2.hpp"

#include "cameracontroller.hpp"
#include "instancemode.hpp"
#include "mask.hpp"
#include "pathgridmode.hpp"

CSVRender::WorldspaceWidget::WorldspaceWidget(CSMDoc::Document& document, QWidget* parent)
    : SceneWidget(document.getData().getResourceSystem(), parent, Qt::WindowFlags(), false)
    , mSceneElements(nullptr)
    , mRun(nullptr)
    , mDocument(document)
    , mInteractionMask(0)
    , mEditMode(nullptr)
    , mLocked(false)
    , mDragMode(InteractionType_None)
    , mDragging(false)
    , mDragX(0)
    , mDragY(0)
    , mSpeedMode(false)
    , mDragFactor(0)
    , mDragWheelFactor(0)
    , mDragShiftFactor(0)
    , mToolTipPos(-1, -1)
    , mShowToolTips(false)
    , mToolTipDelay(0)
    , mSelectedNavigationMode(0)
    , mSelectionMarker(ObjectMarker::create(this, document.getData().getResourceSystem().get()))
{
    setAcceptDrops(true);

    QAbstractItemModel* referenceables = document.getData().getTableModel(CSMWorld::UniversalId::Type_Referenceables);

    connect(referenceables, &QAbstractItemModel::dataChanged, this, &WorldspaceWidget::referenceableDataChanged);
    connect(referenceables, &QAbstractItemModel::rowsAboutToBeRemoved, this,
        &WorldspaceWidget::referenceableAboutToBeRemoved);
    connect(referenceables, &QAbstractItemModel::rowsInserted, this, &WorldspaceWidget::referenceableAdded);

    QAbstractItemModel* references = document.getData().getTableModel(CSMWorld::UniversalId::Type_References);

    connect(references, &QAbstractItemModel::dataChanged, this, &WorldspaceWidget::referenceDataChanged);
    connect(references, &QAbstractItemModel::rowsAboutToBeRemoved, this, &WorldspaceWidget::referenceAboutToBeRemoved);
    connect(references, &QAbstractItemModel::rowsInserted, this, &WorldspaceWidget::referenceAdded);

    QAbstractItemModel* pathgrids = document.getData().getTableModel(CSMWorld::UniversalId::Type_Pathgrids);

    connect(pathgrids, &QAbstractItemModel::dataChanged, this, &WorldspaceWidget::pathgridDataChanged);
    connect(pathgrids, &QAbstractItemModel::rowsAboutToBeRemoved, this, &WorldspaceWidget::pathgridAboutToBeRemoved);
    connect(pathgrids, &QAbstractItemModel::rowsInserted, this, &WorldspaceWidget::pathgridAdded);

    QAbstractItemModel* debugProfiles = document.getData().getTableModel(CSMWorld::UniversalId::Type_DebugProfiles);

    connect(debugProfiles, &QAbstractItemModel::dataChanged, this, &WorldspaceWidget::debugProfileDataChanged);
    connect(debugProfiles, &QAbstractItemModel::rowsAboutToBeRemoved, this,
        &WorldspaceWidget::debugProfileAboutToBeRemoved);

    mToolTipDelayTimer.setSingleShot(true);
    connect(&mToolTipDelayTimer, &QTimer::timeout, this, &WorldspaceWidget::showToolTip);

    CSMPrefs::get()["3D Scene Input"].update();
    CSMPrefs::get()["Tooltips"].update();

    // Shortcuts
    CSMPrefs::Shortcut* primaryEditShortcut
        = new CSMPrefs::Shortcut("scene-edit-primary", "scene-speed-modifier", CSMPrefs::Shortcut::SM_Detach, this);
    CSMPrefs::Shortcut* primaryOpenShortcut = new CSMPrefs::Shortcut("scene-open-primary", this);

    connect(primaryOpenShortcut, qOverload<bool>(&CSMPrefs::Shortcut::activated), this, &WorldspaceWidget::primaryOpen);
    connect(primaryEditShortcut, qOverload<bool>(&CSMPrefs::Shortcut::activated), this, &WorldspaceWidget::primaryEdit);
    connect(primaryEditShortcut, qOverload<bool>(&CSMPrefs::Shortcut::secondary), this, &WorldspaceWidget::speedMode);

    CSMPrefs::Shortcut* secondaryEditShortcut = new CSMPrefs::Shortcut("scene-edit-secondary", this);
    connect(
        secondaryEditShortcut, qOverload<bool>(&CSMPrefs::Shortcut::activated), this, &WorldspaceWidget::secondaryEdit);

    CSMPrefs::Shortcut* primarySelectShortcut = new CSMPrefs::Shortcut("scene-select-primary", this);
    connect(
        primarySelectShortcut, qOverload<bool>(&CSMPrefs::Shortcut::activated), this, &WorldspaceWidget::primarySelect);

    CSMPrefs::Shortcut* secondarySelectShortcut = new CSMPrefs::Shortcut("scene-select-secondary", this);
    connect(secondarySelectShortcut, qOverload<bool>(&CSMPrefs::Shortcut::activated), this,
        &WorldspaceWidget::secondarySelect);

    CSMPrefs::Shortcut* tertiarySelectShortcut = new CSMPrefs::Shortcut("scene-select-tertiary", this);
    connect(tertiarySelectShortcut, qOverload<bool>(&CSMPrefs::Shortcut::activated), this,
        &WorldspaceWidget::tertiarySelect);

    CSMPrefs::Shortcut* abortShortcut = new CSMPrefs::Shortcut("scene-edit-abort", this);
    connect(abortShortcut, qOverload<>(&CSMPrefs::Shortcut::activated), this, &WorldspaceWidget::abortDrag);

    connect(new CSMPrefs::Shortcut("scene-toggle-visibility", this), qOverload<>(&CSMPrefs::Shortcut::activated), this,
        &WorldspaceWidget::toggleHiddenInstances);

    connect(new CSMPrefs::Shortcut("scene-unhide-all", this), qOverload<>(&CSMPrefs::Shortcut::activated), this,
        &WorldspaceWidget::unhideAll);

    connect(new CSMPrefs::Shortcut("scene-clear-selection", this), qOverload<>(&CSMPrefs::Shortcut::activated), this,
        [this] { clearSelection(Mask_Reference); });

    CSMPrefs::Shortcut* switchPerspectiveShortcut = new CSMPrefs::Shortcut("scene-cam-cycle", this);
    connect(switchPerspectiveShortcut, qOverload<>(&CSMPrefs::Shortcut::activated), this,
        &WorldspaceWidget::cycleNavigationMode);

    connect(new CSMPrefs::Shortcut("scene-toggle-marker", this), qOverload<>(&CSMPrefs::Shortcut::activated), this,
        [this]() { mSelectionMarker->toggleVisibility(); });
}

void CSVRender::WorldspaceWidget::settingChanged(const CSMPrefs::Setting* setting)
{
    if (*setting == "3D Scene Input/drag-factor")
        mDragFactor = setting->toDouble();
    else if (*setting == "3D Scene Input/drag-wheel-factor")
        mDragWheelFactor = setting->toDouble();
    else if (*setting == "3D Scene Input/drag-shift-factor")
        mDragShiftFactor = setting->toDouble();
    else if (*setting == "Rendering/object-marker-scale")
        mSelectionMarker->updateScale(setting->toDouble());
    else if (*setting == "Tooltips/scene-delay")
        mToolTipDelay = setting->toInt();
    else if (*setting == "Tooltips/scene")
        mShowToolTips = setting->isTrue();
    else
        SceneWidget::settingChanged(setting);
}

void CSVRender::WorldspaceWidget::useViewHint(const std::string& hint) {}

void CSVRender::WorldspaceWidget::selectDefaultNavigationMode()
{
    selectNavigationMode("1st");
}

void CSVRender::WorldspaceWidget::centerOrbitCameraOnSelection()
{
    std::vector<osg::ref_ptr<TagBase>> selection = getSelection(Mask_Reference);

    for (std::vector<osg::ref_ptr<TagBase>>::iterator it = selection.begin(); it != selection.end(); ++it)
    {
        if (CSVRender::ObjectTag* objectTag = static_cast<CSVRender::ObjectTag*>(it->get()))
        {
            mOrbitCamControl->setCenter(objectTag->mObject->getPosition().asVec3());
        }
    }
}

CSVWidget::SceneToolMode* CSVRender::WorldspaceWidget::makeNavigationSelector(CSVWidget::SceneToolbar* parent)
{
    CSVWidget::SceneToolMode* tool = new CSVWidget::SceneToolMode(parent, "Camera Mode");

    /// \todo replace icons
    /// \todo consider user-defined button-mapping
    tool->addButton(":scenetoolbar/1st-person", "1st",
        "First Person"
        "<ul><li>Camera is held upright</li>"
        "<li>Mouse-Look while holding {scene-navi-primary}</li>"
        "<li>Movement keys: {free-forward}(forward), {free-left}(left), {free-backward}(back), {free-right}(right)</li>"
        "<li>Strafing (also vertically) by holding {scene-navi-secondary}</li>"
        "<li>Mouse wheel moves the camera forward/backward</li>"
        "<li>Hold {scene-speed-modifier} to speed up movement</li>"
        "</ul>");
    tool->addButton(":scenetoolbar/free-camera", "free",
        "Free Camera"
        "<ul><li>Mouse-Look while holding {scene-navi-primary}</li>"
        "<li>Movement keys: {free-forward}(forward), {free-left}(left), {free-backward}(back), {free-right}(right)</li>"
        "<li>Roll camera with {free-roll-left} and {free-roll-right} keys</li>"
        "<li>Strafing (also vertically) by holding {scene-navi-secondary}</li>"
        "<li>Mouse wheel moves the camera forward/backward</li>"
        "<li>Hold {free-forward:mod} to speed up movement</li>"
        "</ul>");
    tool->addButton(
        new CSVRender::OrbitCameraMode(this, Misc::ScalableIcon::load(":scenetoolbar/orbiting-camera"),
            "Orbiting Camera"
            "<ul><li>Always facing the centre point</li>"
            "<li>Rotate around the centre point via {orbit-up}, {orbit-left}, {orbit-down}, {orbit-right} or by moving "
            "the mouse while holding {scene-navi-primary}</li>"
            "<li>Roll camera with {orbit-roll-left} and {orbit-roll-right} keys</li>"
            "<li>Strafing (also vertically) by holding {scene-navi-secondary} (includes relocation of the centre "
            "point)</li>"
            "<li>Mouse wheel moves camera away or towards centre point but can not pass through it</li>"
            "<li>Hold {scene-speed-modifier} to speed up movement</li>"
            "</ul>",
            tool),
        "orbit");

    mCameraMode = tool;

    connect(mCameraMode, &CSVWidget::SceneToolMode::modeChanged, this, &WorldspaceWidget::selectNavigationMode);

    return mCameraMode;
}

CSVWidget::SceneToolToggle2* CSVRender::WorldspaceWidget::makeSceneVisibilitySelector(CSVWidget::SceneToolbar* parent)
{
    mSceneElements = new CSVWidget::SceneToolToggle2(
        parent, "Scene Element Visibility", ":scenetoolbar/scene-view-c", ":scenetoolbar/scene-view-");

    addVisibilitySelectorButtons(mSceneElements);

    mSceneElements->setSelectionMask(0xffffffff);

    connect(mSceneElements, &CSVWidget::SceneToolToggle2::selectionChanged, this,
        &WorldspaceWidget::elementSelectionChanged);

    return mSceneElements;
}

CSVWidget::SceneToolRun* CSVRender::WorldspaceWidget::makeRunTool(CSVWidget::SceneToolbar* parent)
{
    CSMWorld::IdTable& debugProfiles = dynamic_cast<CSMWorld::IdTable&>(
        *mDocument.getData().getTableModel(CSMWorld::UniversalId::Type_DebugProfiles));

    std::vector<std::string> profiles;

    int idColumn = debugProfiles.findColumnIndex(CSMWorld::Columns::ColumnId_Id);
    int stateColumn = debugProfiles.findColumnIndex(CSMWorld::Columns::ColumnId_Modification);
    int defaultColumn = debugProfiles.findColumnIndex(CSMWorld::Columns::ColumnId_DefaultProfile);

    int size = debugProfiles.rowCount();

    for (int i = 0; i < size; ++i)
    {
        int state = debugProfiles.data(debugProfiles.index(i, stateColumn)).toInt();

        bool default_ = debugProfiles.data(debugProfiles.index(i, defaultColumn)).toInt();

        if (state != CSMWorld::RecordBase::State_Deleted && default_)
            profiles.emplace_back(debugProfiles.data(debugProfiles.index(i, idColumn)).toString().toUtf8().constData());
    }

    std::sort(profiles.begin(), profiles.end());

    mRun = new CSVWidget::SceneToolRun(
        parent, "Run OpenMW from the current camera position", ":scenetoolbar/play", profiles);

    connect(mRun, &CSVWidget::SceneToolRun::runRequest, this, &WorldspaceWidget::runRequest);

    return mRun;
}

CSVWidget::SceneToolMode* CSVRender::WorldspaceWidget::makeEditModeSelector(CSVWidget::SceneToolbar* parent)
{
    mEditMode = new CSVWidget::SceneToolMode(parent, "Edit Mode");

    addEditModeSelectorButtons(mEditMode);

    connect(mEditMode, &CSVWidget::SceneToolMode::modeChanged, this, &WorldspaceWidget::editModeChanged);

    return mEditMode;
}

CSVRender::WorldspaceWidget::DropType CSVRender::WorldspaceWidget::getDropType(
    const std::vector<CSMWorld::UniversalId>& data)
{
    DropType output = Type_Other;

    for (std::vector<CSMWorld::UniversalId>::const_iterator iter(data.begin()); iter != data.end(); ++iter)
    {
        DropType type = Type_Other;

        if (iter->getType() == CSMWorld::UniversalId::Type_Cell
            || iter->getType() == CSMWorld::UniversalId::Type_Cell_Missing)
        {
            type = iter->getId()[0] == '#' ? Type_CellsExterior : Type_CellsInterior;
        }
        else if (iter->getType() == CSMWorld::UniversalId::Type_DebugProfile)
            type = Type_DebugProfile;

        if (iter == data.begin())
            output = type;
        else if (output != type) // mixed types -> ignore
            return Type_Other;
    }

    return output;
}

CSVRender::WorldspaceWidget::dropRequirments CSVRender::WorldspaceWidget::getDropRequirements(DropType type) const
{
    if (type == Type_DebugProfile)
        return canHandle;

    return ignored;
}

bool CSVRender::WorldspaceWidget::handleDrop(const std::vector<CSMWorld::UniversalId>& universalIdData, DropType type)
{
    if (type == Type_DebugProfile)
    {
        if (mRun)
        {
            for (std::vector<CSMWorld::UniversalId>::const_iterator iter(universalIdData.begin());
                 iter != universalIdData.end(); ++iter)
                mRun->addProfile(iter->getId());
        }

        return true;
    }

    return false;
}

unsigned int CSVRender::WorldspaceWidget::getVisibilityMask() const
{
    return mSceneElements->getSelectionMask();
}

void CSVRender::WorldspaceWidget::setInteractionMask(unsigned int mask)
{
    mInteractionMask = mask | Mask_CellMarker | Mask_CellArrow;
}

unsigned int CSVRender::WorldspaceWidget::getInteractionMask() const
{
    return mInteractionMask & getVisibilityMask();
}

void CSVRender::WorldspaceWidget::setEditLock(bool locked)
{
    dynamic_cast<CSVRender::EditMode&>(*mEditMode->getCurrent()).setEditLock(locked);
}

void CSVRender::WorldspaceWidget::addVisibilitySelectorButtons(CSVWidget::SceneToolToggle2* tool)
{
    tool->addButton(Button_Reference, Mask_Reference, "Instances");
    tool->addButton(Button_Water, Mask_Water, "Water");
    tool->addButton(Button_Pathgrid, Mask_Pathgrid, "Pathgrid");
}

void CSVRender::WorldspaceWidget::addEditModeSelectorButtons(CSVWidget::SceneToolMode* tool)
{
    /// \todo replace EditMode with suitable subclasses
    tool->addButton(new InstanceMode(this, mRootNode, tool), "object");
    tool->addButton(new PathgridMode(this, tool), "pathgrid");
}

CSMDoc::Document& CSVRender::WorldspaceWidget::getDocument()
{
    return mDocument;
}

template <typename Tag>
std::optional<CSVRender::WorldspaceHitResult> CSVRender::WorldspaceWidget::checkTag(
    const osgUtil::LineSegmentIntersector::Intersection& intersection) const
{
    for (auto* node : intersection.nodePath)
    {
        if (auto* tag = dynamic_cast<Tag*>(node->getUserData()))
        {
            WorldspaceHitResult hit = { true, tag, 0, 0, 0, intersection.getWorldIntersectPoint() };
            if (intersection.indexList.size() >= 3)
            {
                hit.index0 = intersection.indexList[0];
                hit.index1 = intersection.indexList[1];
                hit.index2 = intersection.indexList[2];
            }
            return hit;
        }
    }
    return std::nullopt;
}

std::tuple<osg::Vec3d, osg::Vec3d, osg::Vec3d> CSVRender::WorldspaceWidget::getStartEndDirection(
    int pointX, int pointY) const
{
    // may be okay to just use devicePixelRatio() directly
    QScreen* screen = SceneWidget::windowHandle() && SceneWidget::windowHandle()->screen()
        ? SceneWidget::windowHandle()->screen()
        : QGuiApplication::primaryScreen();

    // (0,0) is considered the lower left corner of an OpenGL window
    int x = pointX * screen->devicePixelRatio();
    int y = height() * screen->devicePixelRatio() - pointY * screen->devicePixelRatio();

    // Convert from screen space to world space
    osg::Matrixd wpvMat;
    wpvMat.preMult(mView->getCamera()->getViewport()->computeWindowMatrix());
    wpvMat.preMult(mView->getCamera()->getProjectionMatrix());
    wpvMat.preMult(mView->getCamera()->getViewMatrix());
    wpvMat = osg::Matrixd::inverse(wpvMat);

    osg::Vec3d start = wpvMat.preMult(osg::Vec3d(x, y, 0));
    osg::Vec3d end = wpvMat.preMult(osg::Vec3d(x, y, 1));
    osg::Vec3d direction = end - start;
    return { start, end, direction };
}

CSVRender::WorldspaceHitResult CSVRender::WorldspaceWidget::mousePick(
    const QPoint& localPos, unsigned int interactionMask) const
{
    auto [start, end, direction] = getStartEndDirection(localPos.x(), localPos.y());

    // Get intersection
    osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector(
        new osgUtil::LineSegmentIntersector(osgUtil::Intersector::MODEL, start, end));

    intersector->setIntersectionLimit(osgUtil::LineSegmentIntersector::NO_LIMIT);
    osgUtil::IntersectionVisitor visitor(intersector);

    visitor.setTraversalMask(interactionMask);

    mView->getCamera()->accept(visitor);

    auto intersections = intersector->getIntersections();

    std::vector<osgUtil::LineSegmentIntersector::Intersection> validIntersections
        = { intersections.begin(), intersections.end() };

    const auto& removeBackfaces = [direction = direction](const osgUtil::LineSegmentIntersector::Intersection& i) {
        return direction * i.getWorldIntersectNormal() > 0;
    };

    validIntersections.erase(std::remove_if(validIntersections.begin(), validIntersections.end(), removeBackfaces),
        validIntersections.end());

    // Default placement
    direction.normalize();
    direction *= CSMPrefs::get()["3D Scene Editing"]["distance"].toInt();

    if (validIntersections.empty())
        return WorldspaceHitResult{ false, nullptr, 0, 0, 0, start + direction };

    const auto& firstHit = validIntersections.front();

    for (const auto& hit : validIntersections)
        if (const auto& markerHit = checkTag<ObjectMarkerTag>(hit))
        {
            if (mSelectionMarker->hitBehindMarker(markerHit->worldPos, mView->getCamera()))
                return WorldspaceHitResult{ false, nullptr, 0, 0, 0, start + direction };
            else
                return *markerHit;
        }
    if (auto hit = checkTag<TagBase>(firstHit))
        return *hit;

    // Something untagged, probably terrain
    WorldspaceHitResult hit = { true, nullptr, 0, 0, 0, firstHit.getWorldIntersectPoint() };
    if (firstHit.indexList.size() >= 3)
    {
        hit.index0 = firstHit.indexList[0];
        hit.index1 = firstHit.indexList[1];
        hit.index2 = firstHit.indexList[2];
    }
    return hit;
}

CSVRender::EditMode* CSVRender::WorldspaceWidget::getEditMode()
{
    return dynamic_cast<CSVRender::EditMode*>(mEditMode->getCurrent());
}

void CSVRender::WorldspaceWidget::abortDrag()
{
    if (mDragging)
    {
        EditMode& editMode = dynamic_cast<CSVRender::EditMode&>(*mEditMode->getCurrent());

        editMode.dragAborted();
        mDragMode = InteractionType_None;
    }
}

void CSVRender::WorldspaceWidget::dragEnterEvent(QDragEnterEvent* event)
{
    const CSMWorld::TableMimeData* mime = dynamic_cast<const CSMWorld::TableMimeData*>(event->mimeData());
    if (!mime) // May happen when non-records (e.g. plain text) are dragged and dropped
        return;

    if (mime->fromDocument(mDocument))
    {
        if (mime->holdsType(CSMWorld::UniversalId::Type_Cell)
            || mime->holdsType(CSMWorld::UniversalId::Type_Cell_Missing)
            || mime->holdsType(CSMWorld::UniversalId::Type_DebugProfile))
        {
            // These drops are handled through the subview object.
            event->accept();
        }
        else
            dynamic_cast<EditMode&>(*mEditMode->getCurrent()).dragEnterEvent(event);
    }
}

void CSVRender::WorldspaceWidget::dragMoveEvent(QDragMoveEvent* event)
{
    const CSMWorld::TableMimeData* mime = dynamic_cast<const CSMWorld::TableMimeData*>(event->mimeData());
    if (!mime) // May happen when non-records (e.g. plain text) are dragged and dropped
        return;

    if (mime->fromDocument(mDocument))
    {
        if (mime->holdsType(CSMWorld::UniversalId::Type_Cell)
            || mime->holdsType(CSMWorld::UniversalId::Type_Cell_Missing)
            || mime->holdsType(CSMWorld::UniversalId::Type_DebugProfile))
        {
            // These drops are handled through the subview object.
            event->accept();
        }
        else
            dynamic_cast<EditMode&>(*mEditMode->getCurrent()).dragMoveEvent(event);
    }
}

void CSVRender::WorldspaceWidget::dropEvent(QDropEvent* event)
{
    const CSMWorld::TableMimeData* mime = dynamic_cast<const CSMWorld::TableMimeData*>(event->mimeData());
    if (!mime) // May happen when non-records (e.g. plain text) are dragged and dropped
        return;

    if (mime->fromDocument(mDocument))
    {
        if (mime->holdsType(CSMWorld::UniversalId::Type_Cell)
            || mime->holdsType(CSMWorld::UniversalId::Type_Cell_Missing)
            || mime->holdsType(CSMWorld::UniversalId::Type_DebugProfile))
        {
            emit dataDropped(mime->getData());
        }
        else
            dynamic_cast<EditMode&>(*mEditMode->getCurrent()).dropEvent(event);
    }
}

void CSVRender::WorldspaceWidget::runRequest(const std::string& profile)
{
    mDocument.startRunning(profile, getStartupInstruction());
}

void CSVRender::WorldspaceWidget::debugProfileDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    if (!mRun)
        return;

    CSMWorld::IdTable& debugProfiles = dynamic_cast<CSMWorld::IdTable&>(
        *mDocument.getData().getTableModel(CSMWorld::UniversalId::Type_DebugProfiles));

    int idColumn = debugProfiles.findColumnIndex(CSMWorld::Columns::ColumnId_Id);
    int stateColumn = debugProfiles.findColumnIndex(CSMWorld::Columns::ColumnId_Modification);

    for (int i = topLeft.row(); i <= bottomRight.row(); ++i)
    {
        int state = debugProfiles.data(debugProfiles.index(i, stateColumn)).toInt();

        // As of version 0.33 this case can not happen because debug profiles exist only in
        // project or session scope, which means they will never be in deleted state. But we
        // are adding the code for the sake of completeness and to avoid surprises if debug
        // profile ever get extended to content scope.
        if (state == CSMWorld::RecordBase::State_Deleted)
            mRun->removeProfile(debugProfiles.data(debugProfiles.index(i, idColumn)).toString().toUtf8().constData());
    }
}

void CSVRender::WorldspaceWidget::debugProfileAboutToBeRemoved(const QModelIndex& parent, int start, int end)
{
    if (parent.isValid())
        return;

    if (!mRun)
        return;

    CSMWorld::IdTable& debugProfiles = dynamic_cast<CSMWorld::IdTable&>(
        *mDocument.getData().getTableModel(CSMWorld::UniversalId::Type_DebugProfiles));

    int idColumn = debugProfiles.findColumnIndex(CSMWorld::Columns::ColumnId_Id);

    for (int i = start; i <= end; ++i)
    {
        mRun->removeProfile(debugProfiles.data(debugProfiles.index(i, idColumn)).toString().toUtf8().constData());
    }
}

void CSVRender::WorldspaceWidget::editModeChanged(const std::string& id)
{
    dynamic_cast<CSVRender::EditMode&>(*mEditMode->getCurrent()).setEditLock(mLocked);
    mDragging = false;
    mDragMode = InteractionType_None;
}

void CSVRender::WorldspaceWidget::showToolTip()
{
    if (mShowToolTips)
    {
        QPoint pos = QCursor::pos();

        WorldspaceHitResult hit = mousePick(mapFromGlobal(pos), getInteractionMask());
        if (hit.tag)
        {
            bool hideBasics = CSMPrefs::get()["Tooltips"]["scene-hide-basic"].isTrue();
            QToolTip::showText(pos, hit.tag->getToolTip(hideBasics, hit), this);
        }
    }
}

void CSVRender::WorldspaceWidget::elementSelectionChanged()
{
    setVisibilityMask(getVisibilityMask());
    flagAsModified();
    updateOverlay();
}

void CSVRender::WorldspaceWidget::updateOverlay() {}

void CSVRender::WorldspaceWidget::handleMarkerHighlight(const int x, const int y)
{
    auto [start, end, _] = getStartEndDirection(x, y);

    osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector(
        new osgUtil::LineSegmentIntersector(osgUtil::Intersector::MODEL, start, end));

    intersector->setIntersectionLimit(osgUtil::LineSegmentIntersector::NO_LIMIT);
    osgUtil::IntersectionVisitor visitor(intersector);

    visitor.setTraversalMask(Mask_Reference);

    mView->getCamera()->accept(visitor);

    bool hitMarker = false;
    for (const auto& intersection : intersector->getIntersections())
    {
        if (mSelectionMarker->hitBehindMarker(intersection.getWorldIntersectPoint(), mView->getCamera()))
            continue;

        for (const auto& node : intersection.nodePath)
        {
            if (const auto& marker = dynamic_cast<ObjectMarkerTag*>(node->getUserData()))
            {
                hitMarker = true;
                mSelectionMarker->updateMarkerHighlight(node->getName(), marker->mAxis);
                break;
            }
        }
    }

    if (!hitMarker)
        mSelectionMarker->resetMarkerHighlight();
}

void CSVRender::WorldspaceWidget::mouseMoveEvent(QMouseEvent* event)
{
    dynamic_cast<CSVRender::EditMode&>(*mEditMode->getCurrent()).mouseMoveEvent(event);

    if (mDragging)
    {
        QPoint pos = event->position().toPoint();
        int diffX = pos.x() - mDragX;
        int diffY = (height() - pos.y()) - mDragY;

        mDragX = pos.x();
        mDragY = height() - pos.y();

        double factor = mDragFactor;

        if (mSpeedMode)
            factor *= mDragShiftFactor;

        EditMode& editMode = dynamic_cast<CSVRender::EditMode&>(*mEditMode->getCurrent());

        editMode.drag(event->position().toPoint(), diffX, diffY, factor);
    }
    else if (mDragMode != InteractionType_None)
    {
        EditMode& editMode = dynamic_cast<CSVRender::EditMode&>(*mEditMode->getCurrent());

        if (mDragMode == InteractionType_PrimaryEdit)
            mDragging = editMode.primaryEditStartDrag(event->position().toPoint());
        else if (mDragMode == InteractionType_SecondaryEdit)
            mDragging = editMode.secondaryEditStartDrag(event->position().toPoint());
        else if (mDragMode == InteractionType_PrimarySelect)
            mDragging = editMode.primarySelectStartDrag(event->position().toPoint());
        else if (mDragMode == InteractionType_SecondarySelect)
            mDragging = editMode.secondarySelectStartDrag(event->position().toPoint());

        if (mDragging)
        {
            mDragX = event->position().x();
            mDragY = height() - event->position().y();
        }
    }
    else
    {
        if (event->globalPosition().toPoint() != mToolTipPos)
        {
            mToolTipPos = event->globalPosition().toPoint();

            if (mShowToolTips)
            {
                QToolTip::hideText();
                mToolTipDelayTimer.start(mToolTipDelay);
            }
        }

        QPoint pos = event->position().toPoint();
        handleMarkerHighlight(pos.x(), pos.y());
        SceneWidget::mouseMoveEvent(event);
    }
}

void CSVRender::WorldspaceWidget::wheelEvent(QWheelEvent* event)
{
    if (mDragging)
    {
        double factor = mDragWheelFactor;

        if (mSpeedMode)
            factor *= mDragShiftFactor;

        EditMode& editMode = dynamic_cast<CSVRender::EditMode&>(*mEditMode->getCurrent());
        editMode.dragWheel(event->angleDelta().y(), factor);
    }
    else
        SceneWidget::wheelEvent(event);
}

void CSVRender::WorldspaceWidget::handleInteractionPress(const WorldspaceHitResult& hit, InteractionType type)
{
    EditMode& editMode = dynamic_cast<CSVRender::EditMode&>(*mEditMode->getCurrent());

    if (type == InteractionType_PrimaryEdit)
        editMode.primaryEditPressed(hit);
    else if (type == InteractionType_SecondaryEdit)
        editMode.secondaryEditPressed(hit);
    else if (type == InteractionType_PrimarySelect)
        editMode.primarySelectPressed(hit);
    else if (type == InteractionType_SecondarySelect)
        editMode.secondarySelectPressed(hit);
    else if (type == InteractionType_TertiarySelect)
        editMode.tertiarySelectPressed(hit);
    else if (type == InteractionType_PrimaryOpen)
        editMode.primaryOpenPressed(hit);
}

void CSVRender::WorldspaceWidget::primaryOpen(bool activate)
{
    handleInteraction(InteractionType_PrimaryOpen, activate);
}

void CSVRender::WorldspaceWidget::primaryEdit(bool activate)
{
    handleInteraction(InteractionType_PrimaryEdit, activate);
}

void CSVRender::WorldspaceWidget::secondaryEdit(bool activate)
{
    handleInteraction(InteractionType_SecondaryEdit, activate);
}

void CSVRender::WorldspaceWidget::primarySelect(bool activate)
{
    handleInteraction(InteractionType_PrimarySelect, activate);
}

void CSVRender::WorldspaceWidget::secondarySelect(bool activate)
{
    handleInteraction(InteractionType_SecondarySelect, activate);
}

void CSVRender::WorldspaceWidget::tertiarySelect(bool activate)
{
    handleInteraction(InteractionType_TertiarySelect, activate);
}

void CSVRender::WorldspaceWidget::speedMode(bool activate)
{
    mSpeedMode = activate;
}

void CSVRender::WorldspaceWidget::toggleHiddenInstances()
{
    const std::vector<osg::ref_ptr<TagBase>> selection = getSelection(Mask_Reference);

    if (selection.empty())
        return;

    const CSVRender::ObjectTag* firstSelection = static_cast<CSVRender::ObjectTag*>(selection.begin()->get());
    assert(firstSelection != nullptr);

    const CSVRender::Mask firstMask
        = firstSelection->mObject->getRootNode()->getNodeMask() == Mask_Hidden ? Mask_Reference : Mask_Hidden;

    for (const auto& object : selection)
        if (const auto objectTag = static_cast<CSVRender::ObjectTag*>(object.get()))
            objectTag->mObject->getRootNode()->setNodeMask(firstMask);
}

void CSVRender::WorldspaceWidget::cycleNavigationMode()
{
    switch (++mSelectedNavigationMode)
    {
        case (CameraMode::FirstPerson):
            mCameraMode->setButton("1st");
            break;
        case (CameraMode::Orbit):
            mCameraMode->setButton("orbit");
            break;
        case (CameraMode::Free):
            mCameraMode->setButton("free");
            break;
        default:
            mCameraMode->setButton("1st");
            mSelectedNavigationMode = 0;
            break;
    }
}

void CSVRender::WorldspaceWidget::handleInteraction(InteractionType type, bool activate)
{
    if (activate)
    {
        if (!mDragging)
            mDragMode = type;
    }
    else
    {
        mDragMode = InteractionType_None;

        if (mDragging)
        {
            EditMode* editMode = getEditMode();
            editMode->dragCompleted(mapFromGlobal(QCursor::pos()));
            mDragging = false;
        }
        else
        {
            WorldspaceHitResult hit = mousePick(mapFromGlobal(QCursor::pos()), getInteractionMask());
            handleInteractionPress(hit, type);
        }
    }
}
