#include "worldspacewidget.hpp"

#include <algorithm>

#include <QEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QApplication>
#include <QToolTip>

#include <osgUtil/LineSegmentIntersector>

#include "../../model/world/universalid.hpp"
#include "../../model/world/idtable.hpp"

#include "../../model/prefs/shortcut.hpp"
#include "../../model/prefs/shortcuteventhandler.hpp"
#include "../../model/prefs/state.hpp"

#include "../render/orbitcameramode.hpp"

#include "../widget/scenetoolmode.hpp"
#include "../widget/scenetooltoggle2.hpp"
#include "../widget/scenetoolrun.hpp"

#include "object.hpp"
#include "mask.hpp"
#include "instancemode.hpp"
#include "pathgridmode.hpp"
#include "cameracontroller.hpp"

CSVRender::WorldspaceWidget::WorldspaceWidget (CSMDoc::Document& document, QWidget* parent)
    : SceneWidget (document.getData().getResourceSystem(), parent, 0, false)
    , mSceneElements(0)
    , mRun(0)
    , mDocument(document)
    , mInteractionMask (0)
    , mEditMode (0)
    , mLocked (false)
    , mDragMode(InteractionType_None)
    , mDragging (false)
    , mDragX(0)
    , mDragY(0)
    , mSpeedMode(false)
    , mDragFactor(0)
    , mDragWheelFactor(0)
    , mDragShiftFactor(0)
    , mToolTipPos (-1, -1)
    , mShowToolTips(false)
    , mToolTipDelay(0)
    , mInConstructor(true)
{
    setAcceptDrops(true);

    QAbstractItemModel *referenceables =
        document.getData().getTableModel (CSMWorld::UniversalId::Type_Referenceables);

    connect (referenceables, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
        this, SLOT (referenceableDataChanged (const QModelIndex&, const QModelIndex&)));
    connect (referenceables, SIGNAL (rowsAboutToBeRemoved (const QModelIndex&, int, int)),
        this, SLOT (referenceableAboutToBeRemoved (const QModelIndex&, int, int)));
    connect (referenceables, SIGNAL (rowsInserted (const QModelIndex&, int, int)),
        this, SLOT (referenceableAdded (const QModelIndex&, int, int)));

    QAbstractItemModel *references =
        document.getData().getTableModel (CSMWorld::UniversalId::Type_References);

    connect (references, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
        this, SLOT (referenceDataChanged (const QModelIndex&, const QModelIndex&)));
    connect (references, SIGNAL (rowsAboutToBeRemoved (const QModelIndex&, int, int)),
        this, SLOT (referenceAboutToBeRemoved (const QModelIndex&, int, int)));
    connect (references, SIGNAL (rowsInserted (const QModelIndex&, int, int)),
        this, SLOT (referenceAdded (const QModelIndex&, int, int)));

    QAbstractItemModel *pathgrids = document.getData().getTableModel (CSMWorld::UniversalId::Type_Pathgrids);

    connect (pathgrids, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
        this, SLOT (pathgridDataChanged (const QModelIndex&, const QModelIndex&)));
    connect (pathgrids, SIGNAL (rowsAboutToBeRemoved (const QModelIndex&, int, int)),
        this, SLOT (pathgridAboutToBeRemoved (const QModelIndex&, int, int)));
    connect (pathgrids, SIGNAL (rowsInserted (const QModelIndex&, int, int)),
        this, SLOT (pathgridAdded (const QModelIndex&, int, int)));

    QAbstractItemModel *debugProfiles =
        document.getData().getTableModel (CSMWorld::UniversalId::Type_DebugProfiles);

    connect (debugProfiles, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
        this, SLOT (debugProfileDataChanged (const QModelIndex&, const QModelIndex&)));
    connect (debugProfiles, SIGNAL (rowsAboutToBeRemoved (const QModelIndex&, int, int)),
        this, SLOT (debugProfileAboutToBeRemoved (const QModelIndex&, int, int)));

    mToolTipDelayTimer.setSingleShot (true);
    connect (&mToolTipDelayTimer, SIGNAL (timeout()), this, SLOT (showToolTip()));

    CSMPrefs::get()["3D Scene Input"].update();
    CSMPrefs::get()["Tooltips"].update();

    // Shortcuts
    CSMPrefs::Shortcut* primaryEditShortcut = new CSMPrefs::Shortcut("scene-edit-primary", "scene-speed-modifier",
            CSMPrefs::Shortcut::SM_Detach, this);
    CSMPrefs::Shortcut* primaryOpenShortcut = new CSMPrefs::Shortcut("scene-open-primary", this);

    connect(primaryOpenShortcut, SIGNAL(activated(bool)), this, SLOT(primaryOpen(bool)));
    connect(primaryEditShortcut, SIGNAL(activated(bool)), this, SLOT(primaryEdit(bool)));
    connect(primaryEditShortcut, SIGNAL(secondary(bool)), this, SLOT(speedMode(bool)));

    CSMPrefs::Shortcut* secondaryEditShortcut = new CSMPrefs::Shortcut("scene-edit-secondary", this);
    connect(secondaryEditShortcut, SIGNAL(activated(bool)), this, SLOT(secondaryEdit(bool)));

    CSMPrefs::Shortcut* primarySelectShortcut = new CSMPrefs::Shortcut("scene-select-primary", this);
    connect(primarySelectShortcut, SIGNAL(activated(bool)), this, SLOT(primarySelect(bool)));

    CSMPrefs::Shortcut* secondarySelectShortcut = new CSMPrefs::Shortcut("scene-select-secondary", this);
    connect(secondarySelectShortcut, SIGNAL(activated(bool)), this, SLOT(secondarySelect(bool)));

    CSMPrefs::Shortcut* abortShortcut = new CSMPrefs::Shortcut("scene-edit-abort", this);
    connect(abortShortcut, SIGNAL(activated()), this, SLOT(abortDrag()));

    mInConstructor = false;
}

CSVRender::WorldspaceWidget::~WorldspaceWidget ()
{
}

void CSVRender::WorldspaceWidget::settingChanged (const CSMPrefs::Setting *setting)
{
    if (*setting=="3D Scene Input/drag-factor")
        mDragFactor = setting->toDouble();
    else if (*setting=="3D Scene Input/drag-wheel-factor")
        mDragWheelFactor = setting->toDouble();
    else if (*setting=="3D Scene Input/drag-shift-factor")
        mDragShiftFactor = setting->toDouble();
    else if (*setting=="Rendering/object-marker-alpha" && !mInConstructor)
    {
        float alpha = setting->toDouble();
        // getSelection is virtual, thus this can not be called from the constructor
        auto selection = getSelection(Mask_Reference);
        for (osg::ref_ptr<TagBase> tag : selection)
        {
            if (auto objTag = dynamic_cast<ObjectTag*>(tag.get()))
                objTag->mObject->setMarkerTransparency(alpha);
        }
    }
    else if (*setting=="Tooltips/scene-delay")
        mToolTipDelay = setting->toInt();
    else if (*setting=="Tooltips/scene")
        mShowToolTips = setting->isTrue();
    else
        SceneWidget::settingChanged(setting);
}


void CSVRender::WorldspaceWidget::useViewHint (const std::string& hint) {}

void CSVRender::WorldspaceWidget::selectDefaultNavigationMode()
{
    selectNavigationMode("1st");
}

void CSVRender::WorldspaceWidget::centerOrbitCameraOnSelection()
{
    std::vector<osg::ref_ptr<TagBase> > selection = getSelection(~0);

    for (std::vector<osg::ref_ptr<TagBase> >::iterator it = selection.begin(); it!=selection.end(); ++it)
    {
        if (CSVRender::ObjectTag *objectTag = dynamic_cast<CSVRender::ObjectTag*> (it->get()))
        {
            mOrbitCamControl->setCenter(objectTag->mObject->getPosition().asVec3());
        }
    }
}

CSVWidget::SceneToolMode *CSVRender::WorldspaceWidget::makeNavigationSelector (
    CSVWidget::SceneToolbar *parent)
{
    CSVWidget::SceneToolMode *tool = new CSVWidget::SceneToolMode (parent, "Camera Mode");

    /// \todo replace icons
    /// \todo consider user-defined button-mapping
    tool->addButton (":scenetoolbar/1st-person", "1st",
        "First Person"
        "<ul><li>Camera is held upright</li>"
        "<li>Mouse-Look while holding {scene-navi-primary}</li>"
        "<li>Movement keys: {free-forward}(forward), {free-left}(left), {free-backward}(back), {free-right}(right)</li>"
        "<li>Strafing (also vertically) by holding {scene-navi-secondary}</li>"
        "<li>Mouse wheel moves the camera forward/backward</li>"
        "<li>Hold {scene-speed-modifier} to speed up movement</li>"
        "</ul>");
    tool->addButton (":scenetoolbar/free-camera", "free",
        "Free Camera"
        "<ul><li>Mouse-Look while holding {scene-navi-primary}</li>"
        "<li>Movement keys: {free-forward}(forward), {free-left}(left), {free-backward}(back), {free-right}(right)</li>"
        "<li>Roll camera with {free-roll-left} and {free-roll-right} keys</li>"
        "<li>Strafing (also vertically) by holding {scene-navi-secondary}</li>"
        "<li>Mouse wheel moves the camera forward/backward</li>"
        "<li>Hold {free-forward:mod} to speed up movement</li>"
        "</ul>");
    tool->addButton(
        new CSVRender::OrbitCameraMode(this, QIcon(":scenetoolbar/orbiting-camera"),
            "Orbiting Camera"
            "<ul><li>Always facing the centre point</li>"
            "<li>Rotate around the centre point via {orbit-up}, {orbit-left}, {orbit-down}, {orbit-right} or by moving "
                "the mouse while holding {scene-navi-primary}</li>"
            "<li>Roll camera with {orbit-roll-left} and {orbit-roll-right} keys</li>"
            "<li>Strafing (also vertically) by holding {scene-navi-secondary} (includes relocation of the centre point)</li>"
            "<li>Mouse wheel moves camera away or towards centre point but can not pass through it</li>"
            "<li>Hold {scene-speed-modifier} to speed up movement</li>"
            "</ul>", tool),
        "orbit");

    connect (tool, SIGNAL (modeChanged (const std::string&)),
        this, SLOT (selectNavigationMode (const std::string&)));

    return tool;
}

CSVWidget::SceneToolToggle2 *CSVRender::WorldspaceWidget::makeSceneVisibilitySelector (CSVWidget::SceneToolbar *parent)
{
    mSceneElements = new CSVWidget::SceneToolToggle2 (parent,
        "Scene Element Visibility", ":scenetoolbar/scene-view-c", ":scenetoolbar/scene-view-");

    addVisibilitySelectorButtons (mSceneElements);

    mSceneElements->setSelectionMask (0xffffffff);

    connect (mSceneElements, SIGNAL (selectionChanged()),
        this, SLOT (elementSelectionChanged()));

    return mSceneElements;
}

CSVWidget::SceneToolRun *CSVRender::WorldspaceWidget::makeRunTool (
    CSVWidget::SceneToolbar *parent)
{
    CSMWorld::IdTable& debugProfiles = dynamic_cast<CSMWorld::IdTable&> (
        *mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_DebugProfiles));

    std::vector<std::string> profiles;

    int idColumn = debugProfiles.findColumnIndex (CSMWorld::Columns::ColumnId_Id);
    int stateColumn = debugProfiles.findColumnIndex (CSMWorld::Columns::ColumnId_Modification);
    int defaultColumn = debugProfiles.findColumnIndex (
        CSMWorld::Columns::ColumnId_DefaultProfile);

    int size = debugProfiles.rowCount();

    for (int i=0; i<size; ++i)
    {
        int state = debugProfiles.data (debugProfiles.index (i, stateColumn)).toInt();

        bool default_ = debugProfiles.data (debugProfiles.index (i, defaultColumn)).toInt();

        if (state!=CSMWorld::RecordBase::State_Deleted && default_)
            profiles.push_back (
                debugProfiles.data (debugProfiles.index (i, idColumn)).
                toString().toUtf8().constData());
    }

    std::sort (profiles.begin(), profiles.end());

    mRun = new CSVWidget::SceneToolRun (parent, "Run OpenMW from the current camera position",
        ":scenetoolbar/play", profiles);

    connect (mRun, SIGNAL (runRequest (const std::string&)),
        this, SLOT (runRequest (const std::string&)));

    return mRun;
}

CSVWidget::SceneToolMode *CSVRender::WorldspaceWidget::makeEditModeSelector (
    CSVWidget::SceneToolbar *parent)
{
    mEditMode = new CSVWidget::SceneToolMode (parent, "Edit Mode");

    addEditModeSelectorButtons (mEditMode);

    connect (mEditMode, SIGNAL (modeChanged (const std::string&)),
        this, SLOT (editModeChanged (const std::string&)));

    return mEditMode;
}

CSVRender::WorldspaceWidget::DropType CSVRender::WorldspaceWidget::getDropType (
    const std::vector< CSMWorld::UniversalId >& data)
{
    DropType output = Type_Other;

    for (std::vector<CSMWorld::UniversalId>::const_iterator iter (data.begin());
        iter!=data.end(); ++iter)
    {
        DropType type = Type_Other;

        if (iter->getType()==CSMWorld::UniversalId::Type_Cell ||
            iter->getType()==CSMWorld::UniversalId::Type_Cell_Missing)
        {
            type = iter->getId().substr (0, 1)=="#" ? Type_CellsExterior : Type_CellsInterior;
        }
        else if (iter->getType()==CSMWorld::UniversalId::Type_DebugProfile)
            type = Type_DebugProfile;

        if (iter==data.begin())
            output = type;
        else if  (output!=type) // mixed types -> ignore
            return Type_Other;
    }

    return output;
}

CSVRender::WorldspaceWidget::dropRequirments
    CSVRender::WorldspaceWidget::getDropRequirements (DropType type) const
{
    if (type==Type_DebugProfile)
        return canHandle;

    return ignored;
}

bool CSVRender::WorldspaceWidget::handleDrop (const std::vector<CSMWorld::UniversalId>& universalIdData,
    DropType type)
{
    if (type==Type_DebugProfile)
    {
        if (mRun)
        {
            for (std::vector<CSMWorld::UniversalId>::const_iterator iter (universalIdData.begin());
                iter!=universalIdData.end(); ++iter)
                mRun->addProfile (iter->getId());
        }

        return true;
    }

    return false;
}

unsigned int CSVRender::WorldspaceWidget::getVisibilityMask() const
{
    return mSceneElements->getSelectionMask();
}

void CSVRender::WorldspaceWidget::setInteractionMask (unsigned int mask)
{
    mInteractionMask = mask | Mask_CellMarker | Mask_CellArrow;
}

unsigned int CSVRender::WorldspaceWidget::getInteractionMask() const
{
    return mInteractionMask & getVisibilityMask();
}

void CSVRender::WorldspaceWidget::setEditLock (bool locked)
{
    dynamic_cast<CSVRender::EditMode&> (*mEditMode->getCurrent()).setEditLock (locked);
}

void CSVRender::WorldspaceWidget::addVisibilitySelectorButtons (
    CSVWidget::SceneToolToggle2 *tool)
{
    tool->addButton (Button_Reference, Mask_Reference, "Instances");
    tool->addButton (Button_Water, Mask_Water, "Water");
    tool->addButton (Button_Pathgrid, Mask_Pathgrid, "Pathgrid");
}

void CSVRender::WorldspaceWidget::addEditModeSelectorButtons (CSVWidget::SceneToolMode *tool)
{
    /// \todo replace EditMode with suitable subclasses
    tool->addButton (new InstanceMode (this, mRootNode, tool), "object");
    tool->addButton (new PathgridMode (this, tool), "pathgrid");
}

CSMDoc::Document& CSVRender::WorldspaceWidget::getDocument()
{
    return mDocument;
}

CSVRender::WorldspaceHitResult CSVRender::WorldspaceWidget::mousePick (const QPoint& localPos,
    unsigned int interactionMask) const
{
    // (0,0) is considered the lower left corner of an OpenGL window
    int x = localPos.x();
    int y = height() - localPos.y();

    // Convert from screen space to world space
    osg::Matrixd wpvMat;
    wpvMat.preMult (mView->getCamera()->getViewport()->computeWindowMatrix());
    wpvMat.preMult (mView->getCamera()->getProjectionMatrix());
    wpvMat.preMult (mView->getCamera()->getViewMatrix());
    wpvMat = osg::Matrixd::inverse (wpvMat);

    osg::Vec3d start = wpvMat.preMult (osg::Vec3d(x, y, 0));
    osg::Vec3d end = wpvMat.preMult (osg::Vec3d(x, y, 1));
    osg::Vec3d direction = end - start;

    // Get intersection
    osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector (new osgUtil::LineSegmentIntersector(
        osgUtil::Intersector::MODEL, start, end));

    intersector->setIntersectionLimit(osgUtil::LineSegmentIntersector::NO_LIMIT);
    osgUtil::IntersectionVisitor visitor(intersector);

    visitor.setTraversalMask(interactionMask);

    mView->getCamera()->accept(visitor);

    // Get relevant data
    for (osgUtil::LineSegmentIntersector::Intersections::iterator it = intersector->getIntersections().begin();
         it != intersector->getIntersections().end(); ++it)
    {
        osgUtil::LineSegmentIntersector::Intersection intersection = *it;

        // reject back-facing polygons
        if (direction * intersection.getWorldIntersectNormal() > 0)
        {
            continue;
        }

        for (std::vector<osg::Node*>::iterator nodeIter = intersection.nodePath.begin(); nodeIter != intersection.nodePath.end(); ++nodeIter)
        {
            osg::Node* node = *nodeIter;
            if (osg::ref_ptr<CSVRender::TagBase> tag = dynamic_cast<CSVRender::TagBase *>(node->getUserData()))
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

        // Something untagged, probably terrain
        WorldspaceHitResult hit = { true, 0, 0, 0, 0, intersection.getWorldIntersectPoint() };
        if (intersection.indexList.size() >= 3)
        {
            hit.index0 = intersection.indexList[0];
            hit.index1 = intersection.indexList[1];
            hit.index2 = intersection.indexList[2];
        }
        return hit;
    }

    // Default placement
    direction.normalize();
    direction *= CSMPrefs::get()["3D Scene Editing"]["distance"].toInt();

    WorldspaceHitResult hit = { false, 0, 0, 0, 0, start + direction };
    return hit;
}

void CSVRender::WorldspaceWidget::abortDrag()
{
    if (mDragging)
    {
        EditMode& editMode = dynamic_cast<CSVRender::EditMode&> (*mEditMode->getCurrent());

        editMode.dragAborted();
        mDragging = false;
        mDragMode = InteractionType_None;
    }
}

void CSVRender::WorldspaceWidget::dragEnterEvent (QDragEnterEvent* event)
{
    const CSMWorld::TableMimeData* mime = dynamic_cast<const CSMWorld::TableMimeData*> (event->mimeData());
    if (!mime) // May happen when non-records (e.g. plain text) are dragged and dropped
        return;

    if (mime->fromDocument (mDocument))
    {
        if (mime->holdsType (CSMWorld::UniversalId::Type_Cell) ||
            mime->holdsType (CSMWorld::UniversalId::Type_Cell_Missing) ||
            mime->holdsType (CSMWorld::UniversalId::Type_DebugProfile))
        {
            // These drops are handled through the subview object.
            event->accept();
        }
        else
            dynamic_cast<EditMode&> (*mEditMode->getCurrent()).dragEnterEvent (event);
    }
}

void CSVRender::WorldspaceWidget::dragMoveEvent(QDragMoveEvent *event)
{
    const CSMWorld::TableMimeData* mime = dynamic_cast<const CSMWorld::TableMimeData*> (event->mimeData());
    if (!mime) // May happen when non-records (e.g. plain text) are dragged and dropped
        return;

    if (mime->fromDocument (mDocument))
    {
        if (mime->holdsType (CSMWorld::UniversalId::Type_Cell) ||
            mime->holdsType (CSMWorld::UniversalId::Type_Cell_Missing) ||
            mime->holdsType (CSMWorld::UniversalId::Type_DebugProfile))
        {
            // These drops are handled through the subview object.
            event->accept();
        }
        else
            dynamic_cast<EditMode&> (*mEditMode->getCurrent()).dragMoveEvent (event);
    }
}

void CSVRender::WorldspaceWidget::dropEvent (QDropEvent* event)
{
    const CSMWorld::TableMimeData* mime = dynamic_cast<const CSMWorld::TableMimeData*> (event->mimeData());
    if (!mime) // May happen when non-records (e.g. plain text) are dragged and dropped
        return;

    if (mime->fromDocument (mDocument))
    {
        if (mime->holdsType (CSMWorld::UniversalId::Type_Cell) ||
            mime->holdsType (CSMWorld::UniversalId::Type_Cell_Missing) ||
            mime->holdsType (CSMWorld::UniversalId::Type_DebugProfile))
        {
            emit dataDropped(mime->getData());
        }
        else
            dynamic_cast<EditMode&> (*mEditMode->getCurrent()).dropEvent (event);
    }
}

void CSVRender::WorldspaceWidget::runRequest (const std::string& profile)
{
    mDocument.startRunning (profile, getStartupInstruction());
}

void CSVRender::WorldspaceWidget::debugProfileDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    if (!mRun)
        return;

    CSMWorld::IdTable& debugProfiles = dynamic_cast<CSMWorld::IdTable&> (
        *mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_DebugProfiles));

    int idColumn = debugProfiles.findColumnIndex (CSMWorld::Columns::ColumnId_Id);
    int stateColumn = debugProfiles.findColumnIndex (CSMWorld::Columns::ColumnId_Modification);

    for (int i=topLeft.row(); i<=bottomRight.row(); ++i)
    {
        int state = debugProfiles.data (debugProfiles.index (i, stateColumn)).toInt();

        // As of version 0.33 this case can not happen because debug profiles exist only in
        // project or session scope, which means they will never be in deleted state. But we
        // are adding the code for the sake of completeness and to avoid surprises if debug
        // profile ever get extended to content scope.
        if (state==CSMWorld::RecordBase::State_Deleted)
            mRun->removeProfile (debugProfiles.data (
                debugProfiles.index (i, idColumn)).toString().toUtf8().constData());
    }
}

void CSVRender::WorldspaceWidget::debugProfileAboutToBeRemoved (const QModelIndex& parent,
    int start, int end)
{
    if (parent.isValid())
        return;

    if (!mRun)
        return;

    CSMWorld::IdTable& debugProfiles = dynamic_cast<CSMWorld::IdTable&> (
        *mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_DebugProfiles));

    int idColumn = debugProfiles.findColumnIndex (CSMWorld::Columns::ColumnId_Id);

    for (int i=start; i<=end; ++i)
    {
        mRun->removeProfile (debugProfiles.data (
            debugProfiles.index (i, idColumn)).toString().toUtf8().constData());
    }
}

void CSVRender::WorldspaceWidget::editModeChanged (const std::string& id)
{
    dynamic_cast<CSVRender::EditMode&> (*mEditMode->getCurrent()).setEditLock (mLocked);
    mDragging = false;
    mDragMode = InteractionType_None;
}

void CSVRender::WorldspaceWidget::showToolTip()
{
    if (mShowToolTips)
    {
        QPoint pos = QCursor::pos();

        WorldspaceHitResult hit = mousePick (mapFromGlobal (pos), getInteractionMask());
        if (hit.tag)
        {
            bool hideBasics = CSMPrefs::get()["Tooltips"]["scene-hide-basic"].isTrue();
            QToolTip::showText (pos, hit.tag->getToolTip (hideBasics), this);
        }
    }
}

void CSVRender::WorldspaceWidget::elementSelectionChanged()
{
    setVisibilityMask (getVisibilityMask());
    flagAsModified();
    updateOverlay();
}

void CSVRender::WorldspaceWidget::updateOverlay()
{
}

void CSVRender::WorldspaceWidget::mouseMoveEvent (QMouseEvent *event)
{
    dynamic_cast<CSVRender::EditMode&> (*mEditMode->getCurrent()).mouseMoveEvent (event);

    if (mDragging)
    {
        int diffX = event->x() - mDragX;
        int diffY = (height() - event->y()) - mDragY;

        mDragX = event->x();
        mDragY = height() - event->y();

        double factor = mDragFactor;

        if (mSpeedMode)
            factor *= mDragShiftFactor;

        EditMode& editMode = dynamic_cast<CSVRender::EditMode&> (*mEditMode->getCurrent());

        editMode.drag (event->pos(), diffX, diffY, factor);
    }
    else if (mDragMode != InteractionType_None)
    {
        EditMode& editMode = dynamic_cast<CSVRender::EditMode&> (*mEditMode->getCurrent());

        if (mDragMode == InteractionType_PrimaryEdit)
            mDragging = editMode.primaryEditStartDrag (event->pos());
        else if (mDragMode == InteractionType_SecondaryEdit)
            mDragging = editMode.secondaryEditStartDrag (event->pos());
        else if (mDragMode == InteractionType_PrimarySelect)
            mDragging = editMode.primarySelectStartDrag (event->pos());
        else if (mDragMode == InteractionType_SecondarySelect)
            mDragging = editMode.secondarySelectStartDrag (event->pos());

        if (mDragging)
        {
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
            mDragX = event->localPos().x();
            mDragY = height() - event->localPos().y();
#else
            mDragX = event->posF().x();
            mDragY = height() - event->posF().y();
#endif
        }
    }
    else
    {
        if (event->globalPos()!=mToolTipPos)
        {
            mToolTipPos = event->globalPos();

            if (mShowToolTips)
            {
                QToolTip::hideText();
                mToolTipDelayTimer.start (mToolTipDelay);
            }
        }

        SceneWidget::mouseMoveEvent(event);
    }
}

void CSVRender::WorldspaceWidget::wheelEvent (QWheelEvent *event)
{
    if (mDragging)
    {
        double factor = mDragWheelFactor;

        if (mSpeedMode)
            factor *= mDragShiftFactor;

        EditMode& editMode = dynamic_cast<CSVRender::EditMode&> (*mEditMode->getCurrent());

        editMode.dragWheel (event->delta(), factor);
    }
    else
        SceneWidget::wheelEvent(event);
}

void CSVRender::WorldspaceWidget::handleInteractionPress (const WorldspaceHitResult& hit, InteractionType type)
{
    EditMode& editMode = dynamic_cast<CSVRender::EditMode&> (*mEditMode->getCurrent());

    if (type == InteractionType_PrimaryEdit)
        editMode.primaryEditPressed (hit);
    else if (type == InteractionType_SecondaryEdit)
        editMode.secondaryEditPressed (hit);
    else if (type == InteractionType_PrimarySelect)
        editMode.primarySelectPressed (hit);
    else if (type == InteractionType_SecondarySelect)
        editMode.secondarySelectPressed (hit);
    else if (type == InteractionType_PrimaryOpen)
        editMode.primaryOpenPressed (hit);
}

CSVRender::EditMode *CSVRender::WorldspaceWidget::getEditMode()
{
    return dynamic_cast<CSVRender::EditMode *> (mEditMode->getCurrent());
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

void CSVRender::WorldspaceWidget::speedMode(bool activate)
{
    mSpeedMode = activate;
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
