#include "worldspacewidget.hpp"

#include <algorithm>
#include <iostream>

#include <QEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QApplication>
#include <QToolTip>

#include <osgGA/TrackballManipulator>
#include <osgGA/FirstPersonManipulator>

#include <osgUtil/LineSegmentIntersector>

#include "../../model/world/universalid.hpp"
#include "../../model/world/idtable.hpp"

#include "../../model/settings/usersettings.hpp"

#include "../widget/scenetoolmode.hpp"
#include "../widget/scenetooltoggle2.hpp"
#include "../widget/scenetoolrun.hpp"

#include "object.hpp"
#include "elements.hpp"
#include "editmode.hpp"
#include "instancemode.hpp"

namespace
{
    static const char * const sMappingSettings[] =
    {
        "p-navi", "s-navi",
        "p-edit", "s-edit",
        "p-select", "s-select",
        0
    };
}

CSVRender::WorldspaceWidget::WorldspaceWidget (CSMDoc::Document& document, QWidget* parent)
: SceneWidget (document.getData().getResourceSystem(), parent), mSceneElements(0), mRun(0), mDocument(document),
  mInteractionMask (0), mEditMode (0), mLocked (false), mDragging (false),
  mToolTipPos (-1, -1)
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

    QAbstractItemModel *debugProfiles =
        document.getData().getTableModel (CSMWorld::UniversalId::Type_DebugProfiles);

    connect (debugProfiles, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
        this, SLOT (debugProfileDataChanged (const QModelIndex&, const QModelIndex&)));
    connect (debugProfiles, SIGNAL (rowsAboutToBeRemoved (const QModelIndex&, int, int)),
        this, SLOT (debugProfileAboutToBeRemoved (const QModelIndex&, int, int)));

    for (int i=0; sMappingSettings[i]; ++i)
    {
        QString key ("scene-input/");
        key += sMappingSettings[i];
        storeMappingSetting (key, CSMSettings::UserSettings::instance().settingValue (key));
    }

    mDragFactor = CSMSettings::UserSettings::instance().settingValue ("scene-input/drag-factor").toDouble();
    mDragWheelFactor = CSMSettings::UserSettings::instance().settingValue ("scene-input/drag-wheel-factor").toDouble();
    mDragShiftFactor = CSMSettings::UserSettings::instance().settingValue ("scene-input/drag-shift-factor").toDouble();

    mShowToolTips = CSMSettings::UserSettings::instance().settingValue ("tooltips/scene") == "true";
    mToolTipDelay = CSMSettings::UserSettings::instance().settingValue ("tooltips/scene-delay").toInt();

    mToolTipDelayTimer.setSingleShot (true);
    connect (&mToolTipDelayTimer, SIGNAL (timeout()), this, SLOT (showToolTip()));
}

CSVRender::WorldspaceWidget::~WorldspaceWidget ()
{
}

void CSVRender::WorldspaceWidget::selectNavigationMode (const std::string& mode)
{
    if (mode=="1st")
        mView->setCameraManipulator(new osgGA::FirstPersonManipulator);
    else if (mode=="free")
        mView->setCameraManipulator(new osgGA::FirstPersonManipulator);
    else if (mode=="orbit")
        mView->setCameraManipulator(new osgGA::OrbitManipulator);
}

void CSVRender::WorldspaceWidget::useViewHint (const std::string& hint) {}

void CSVRender::WorldspaceWidget::selectDefaultNavigationMode()
{
    mView->setCameraManipulator(new osgGA::FirstPersonManipulator);
}

CSVWidget::SceneToolMode *CSVRender::WorldspaceWidget::makeNavigationSelector (
    CSVWidget::SceneToolbar *parent)
{
    CSVWidget::SceneToolMode *tool = new CSVWidget::SceneToolMode (parent, "Camera Mode");

    /// \todo replace icons
    /// \todo consider user-defined button-mapping
    tool->addButton (":scenetoolbar/1st-person", "1st",
        "First Person"
        "<ul><li>Mouse-Look while holding the left button</li>"
        "<li>WASD movement keys</li>"
        "<li>Mouse wheel moves the camera forawrd/backward</li>"
        "<li>Stafing (also vertically) by holding the left mouse button and control</li>"
        "<li>Camera is held upright</li>"
        "<li>Hold shift to speed up movement</li>"
        "</ul>");
    tool->addButton (":scenetoolbar/free-camera", "free",
        "Free Camera"
        "<ul><li>Mouse-Look while holding the left button</li>"
        "<li>Stafing (also vertically) via WASD or by holding the left mouse button and control</li>"
        "<li>Mouse wheel moves the camera forawrd/backward</li>"
        "<li>Roll camera with Q and E keys</li>"
        "<li>Hold shift to speed up movement</li>"
        "</ul>");
    tool->addButton (":scenetoolbar/orbiting-camera", "orbit",
        "Orbiting Camera"
        "<ul><li>Always facing the centre point</li>"
        "<li>Rotate around the centre point via WASD or by moving the mouse while holding the left button</li>"
        "<li>Mouse wheel moves camera away or towards centre point but can not pass through it</li>"
        "<li>Roll camera with Q and E keys</li>"
        "<li>Stafing (also vertically) by holding the left mouse button and control (includes relocation of the centre point)</li>"
        "<li>Hold shift to speed up movement</li>"
        "</ul>");

    connect (tool, SIGNAL (modeChanged (const std::string&)),
        this, SLOT (selectNavigationMode (const std::string&)));

    return tool;
}

CSVWidget::SceneToolToggle2 *CSVRender::WorldspaceWidget::makeSceneVisibilitySelector (CSVWidget::SceneToolbar *parent)
{
    mSceneElements = new CSVWidget::SceneToolToggle2 (parent,
        "Scene Element Visibility", ":scenetoolbar/scene-view-c", ":scenetoolbar/scene-view-");

    addVisibilitySelectorButtons (mSceneElements);

    mSceneElements->setSelection (0xffffffff);

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

bool CSVRender::WorldspaceWidget::handleDrop (const std::vector<CSMWorld::UniversalId>& data,
    DropType type)
{
    if (type==Type_DebugProfile)
    {
        if (mRun)
        {
            for (std::vector<CSMWorld::UniversalId>::const_iterator iter (data.begin());
                iter!=data.end(); ++iter)
                mRun->addProfile (iter->getId());
        }

        return true;
    }

    return false;
}

unsigned int CSVRender::WorldspaceWidget::getVisibilityMask() const
{
    return mSceneElements->getSelection();
}

void CSVRender::WorldspaceWidget::setInteractionMask (unsigned int mask)
{
    mInteractionMask = mask | Element_CellMarker | Element_CellArrow;
}

unsigned int CSVRender::WorldspaceWidget::getInteractionMask() const
{
    return mInteractionMask & getVisibilityMask();
}

void CSVRender::WorldspaceWidget::updateUserSetting (const QString& name, const QStringList& value)
{
    if (!value.isEmpty() && storeMappingSetting (name, value.first()))
        return;

    if (name=="scene-input/drag-factor")
        mDragFactor = value.at (0).toDouble();
    else if (name=="scene-input/drag-wheel-factor")
        mDragWheelFactor = value.at (0).toDouble();
    else if (name=="scene-input/drag-shift-factor")
        mDragShiftFactor = value.at (0).toDouble();
    else if (name=="tooltips/scene-delay")
        mToolTipDelay = value.at (0).toInt();
    else if (name=="tooltips/scene")
        mShowToolTips = value.at (0)=="true";
    else
        dynamic_cast<CSVRender::EditMode&> (*mEditMode->getCurrent()).updateUserSetting (name, value);
}

void CSVRender::WorldspaceWidget::setEditLock (bool locked)
{
    dynamic_cast<CSVRender::EditMode&> (*mEditMode->getCurrent()).setEditLock (locked);
}

void CSVRender::WorldspaceWidget::addVisibilitySelectorButtons (
    CSVWidget::SceneToolToggle2 *tool)
{
    tool->addButton (Element_Reference, "Instances");
    tool->addButton (Element_Water, "Water");
    tool->addButton (Element_Pathgrid, "Pathgrid");
}

void CSVRender::WorldspaceWidget::addEditModeSelectorButtons (CSVWidget::SceneToolMode *tool)
{
    /// \todo replace EditMode with suitable subclasses
    tool->addButton (new InstanceMode (this, tool), "object");
    tool->addButton (
        new EditMode (this, QIcon (":placeholder"), Element_Pathgrid, "Pathgrid editing"),
        "pathgrid");
}

CSMDoc::Document& CSVRender::WorldspaceWidget::getDocument()
{
    return mDocument;
}

void CSVRender::WorldspaceWidget::dragEnterEvent (QDragEnterEvent* event)
{
    event->accept();
}

void CSVRender::WorldspaceWidget::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}


bool CSVRender::WorldspaceWidget::storeMappingSetting (const QString& key, const QString& value)
{
    const QString prefix = "scene-input/";

    if (key.startsWith (prefix))
    {
        QString key2 (key.mid (prefix.length()));

        for (int i=0; sMappingSettings[i]; ++i)
            if (key2==sMappingSettings[i])
            {
                Qt::MouseButton button = Qt::NoButton;

                if (value.endsWith ("Left Mouse-Button"))
                    button = Qt::LeftButton;
                else if (value.endsWith ("Right Mouse-Button"))
                    button = Qt::RightButton;
                else if (value.endsWith ("Middle Mouse-Button"))
                    button = Qt::MiddleButton;
                else
                    return false;

                bool ctrl = value.startsWith ("Ctrl-");

                mButtonMapping[std::make_pair (button, ctrl)] = sMappingSettings[i];
                return true;
            }
    }

    return false;
}

osg::ref_ptr<CSVRender::TagBase> CSVRender::WorldspaceWidget::mousePick (const QPoint& localPos)
{
    // (0,0) is considered the lower left corner of an OpenGL window
    int x = localPos.x();
    int y = height() - localPos.y();

    osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector (new osgUtil::LineSegmentIntersector(osgUtil::Intersector::WINDOW, x, y));

    intersector->setIntersectionLimit(osgUtil::LineSegmentIntersector::NO_LIMIT);
    osgUtil::IntersectionVisitor visitor(intersector);

    visitor.setTraversalMask(getInteractionMask() << 1);

    mView->getCamera()->accept(visitor);

    for (osgUtil::LineSegmentIntersector::Intersections::iterator it = intersector->getIntersections().begin();
         it != intersector->getIntersections().end(); ++it)
    {
        osgUtil::LineSegmentIntersector::Intersection intersection = *it;

        // reject back-facing polygons
        osg::Vec3f normal = intersection.getWorldIntersectNormal();
        normal = osg::Matrix::transform3x3(normal, mView->getCamera()->getViewMatrix());
        if (normal.z() < 0)
            continue;

        for (std::vector<osg::Node*>::iterator it = intersection.nodePath.begin(); it != intersection.nodePath.end(); ++it)
        {
            osg::Node* node = *it;
            if (osg::ref_ptr<CSVRender::TagBase> tag = dynamic_cast<CSVRender::TagBase *>(node->getUserData()))
                return tag;
        }

// ignoring terrain for now
        // must be terrain, report coordinates
//        std::cout << "Terrain hit at " << intersection.getWorldIntersectPoint().x() << " " << intersection.getWorldIntersectPoint().y() << std::endl;
//        return;
    }

    return osg::ref_ptr<CSVRender::TagBase>();
}

std::string CSVRender::WorldspaceWidget::mapButton (QMouseEvent *event)
{
    std::pair<Qt::MouseButton, bool> phyiscal (
        event->button(), event->modifiers() & Qt::ControlModifier);

    std::map<std::pair<Qt::MouseButton, bool>, std::string>::const_iterator iter =
        mButtonMapping.find (phyiscal);

    if (iter!=mButtonMapping.end())
        return iter->second;

    return "";
}

void CSVRender::WorldspaceWidget::dropEvent (QDropEvent* event)
{
    const CSMWorld::TableMimeData* mime = dynamic_cast<const CSMWorld::TableMimeData*> (event->mimeData());
    if (!mime) // May happen when non-records (e.g. plain text) are dragged and dropped
        return;

    if (mime->fromDocument (mDocument))
    {
        emit dataDropped(mime->getData());
    } //not handling drops from different documents at the moment
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
}

void CSVRender::WorldspaceWidget::showToolTip()
{
    if (mShowToolTips)
    {
        QPoint pos = QCursor::pos();

        if (osg::ref_ptr<TagBase> tag = mousePick (mapFromGlobal (pos)))
        {
            bool hideBasics =
                CSMSettings::UserSettings::instance().settingValue ("tooltips/scene-hide-basic")=="true";
            QToolTip::showText (pos, tag->getToolTip (hideBasics), this);
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
    if (!mDragging)
    {
        if (mDragMode.empty())
        {
            if (event->globalPos()!=mToolTipPos)
            {
                mToolTipPos = event->globalPos();

                if (mShowToolTips)
                    mToolTipDelayTimer.start (mToolTipDelay);
            }
        }
        else if (mDragMode=="p-navi" || mDragMode=="s-navi")
        {

        }
        else if (mDragMode=="p-edit" || mDragMode=="s-edit" || mDragMode=="p-select" || mDragMode=="s-select")
        {
            osg::ref_ptr<TagBase> tag = mousePick (event->pos());

            EditMode& editMode = dynamic_cast<CSVRender::EditMode&> (*mEditMode->getCurrent());

            if (mDragMode=="p-edit")
                mDragging = editMode.primaryEditStartDrag (tag);
            else if (mDragMode=="s-edit")
                mDragging = editMode.secondaryEditStartDrag (tag);
            else if (mDragMode=="p-select")
                mDragging = editMode.primarySelectStartDrag (tag);
            else if (mDragMode=="s-select")
                mDragging = editMode.secondarySelectStartDrag (tag);

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
    }
    else
    {
        int diffX = event->x() - mDragX;
        int diffY = (height() - event->y()) - mDragY;

        mDragX = event->x();
        mDragY = height() - event->y();

        double factor = mDragFactor;

        if (event->modifiers() & Qt::ShiftModifier)
            factor *= mDragShiftFactor;

        EditMode& editMode = dynamic_cast<CSVRender::EditMode&> (*mEditMode->getCurrent());

        editMode.drag (diffX, diffY, factor);
    }
}

void CSVRender::WorldspaceWidget::mousePressEvent (QMouseEvent *event)
{
    std::string button = mapButton (event);

    if (!mDragging)
        mDragMode = button;
}

void CSVRender::WorldspaceWidget::mouseReleaseEvent (QMouseEvent *event)
{
    std::string button = mapButton (event);

    if (mDragging)
    {
        if (mDragMode=="p-navi" || mDragMode=="s-navi")
        {

        }
        else if (mDragMode=="p-edit" || mDragMode=="s-edit" ||
            mDragMode=="p-select" || mDragMode=="s-select")
        {
            EditMode& editMode = dynamic_cast<CSVRender::EditMode&> (*mEditMode->getCurrent());

            editMode.dragCompleted();
            mDragging = false;
        }
    }
    else
    {
        if (button=="p-navi" || button=="s-navi")
        {

        }
        else if (button=="p-edit" || button=="s-edit" ||
            button=="p-select" || button=="s-select")
        {
            osg::ref_ptr<TagBase> tag = mousePick (event->pos());

            handleMouseClick (tag, button, event->modifiers() & Qt::ShiftModifier);
        }
    }

    mDragMode.clear();
}

void CSVRender::WorldspaceWidget::mouseDoubleClickEvent (QMouseEvent *event)
{
    if(event->button() == Qt::RightButton)
    {
        //mMouse->mouseDoubleClickEvent(event);
    }
}

void CSVRender::WorldspaceWidget::wheelEvent (QWheelEvent *event)
{
    if (mDragging)
    {
        double factor = mDragWheelFactor;

        if (event->modifiers() & Qt::ShiftModifier)
            factor *= mDragShiftFactor;

        EditMode& editMode = dynamic_cast<CSVRender::EditMode&> (*mEditMode->getCurrent());

        editMode.dragWheel (event->delta(), factor);
    }
}

void CSVRender::WorldspaceWidget::keyPressEvent (QKeyEvent *event)
{
    if(event->key() == Qt::Key_Escape)
    {
        if (mDragging)
        {
            EditMode& editMode = dynamic_cast<CSVRender::EditMode&> (*mEditMode->getCurrent());

            editMode.dragAborted();
            mDragging = false;
        }
    }
    else
        RenderWidget::keyPressEvent(event);
}

void CSVRender::WorldspaceWidget::handleMouseClick (osg::ref_ptr<TagBase> tag, const std::string& button, bool shift)
{
    EditMode& editMode = dynamic_cast<CSVRender::EditMode&> (*mEditMode->getCurrent());

    if (button=="p-edit")
        editMode.primaryEditPressed (tag);
    else if (button=="s-edit")
        editMode.secondaryEditPressed (tag);
    else if (button=="p-select")
        editMode.primarySelectPressed (tag);
    else if (button=="s-select")
        editMode.secondarySelectPressed (tag);
}
