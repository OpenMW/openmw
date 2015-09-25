#include "worldspacewidget.hpp"

#include <algorithm>
#include <iostream>

#include <QEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QKeyEvent>

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

namespace
{
    static const char * const sMappingSettings[] =
    {
        "p-navi", "s-navi",
        "p-edit", "s-edit",
        "select",
        0
    };
}

CSVRender::WorldspaceWidget::WorldspaceWidget (CSMDoc::Document& document, QWidget* parent)
: SceneWidget (document.getData().getResourceSystem(), parent), mSceneElements(0), mRun(0), mDocument(document),
  mInteractionMask (0)
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
    CSVWidget::SceneToolMode *tool = new CSVWidget::SceneToolMode (parent, "Edit Mode");

    addEditModeSelectorButtons (tool);

    return tool;
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
    tool->addButton (
        new EditMode (this, QIcon (":placeholder"), Element_Reference, "Instance editing"),
        "object");
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
    if(event->buttons() & Qt::RightButton)
    {
        //mMouse->mouseMoveEvent(event);
    }
    RenderWidget::mouseMoveEvent(event);
}

void CSVRender::WorldspaceWidget::mousePressEvent (QMouseEvent *event)
{
    if (event->button() != Qt::RightButton)
        return;

    // (0,0) is considered the lower left corner of an OpenGL window
    int x = event->x();
    int y = height() - event->y();

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
            if (CSVRender::ObjectHolder* holder = dynamic_cast<CSVRender::ObjectHolder*>(node->getUserData()))
            {
                // hit an Object, toggle its selection state
                CSVRender::Object* obj = holder->mObject;
                obj->setSelected(!obj->getSelected());
                return;
            }
        }

        // must be terrain, report coordinates
        std::cout << "Terrain hit at " << intersection.getWorldIntersectPoint().x() << " " << intersection.getWorldIntersectPoint().y() << std::endl;
        return;
    }
}

void CSVRender::WorldspaceWidget::mouseReleaseEvent (QMouseEvent *event)
{
    if(event->button() == Qt::RightButton)
    {
        /*
        if(!getViewport())
        {
            SceneWidget::mouseReleaseEvent(event);
            return;
        }
        */
        //mMouse->mouseReleaseEvent(event);
    }
    RenderWidget::mouseReleaseEvent(event);
}

void CSVRender::WorldspaceWidget::mouseDoubleClickEvent (QMouseEvent *event)
{
    if(event->button() == Qt::RightButton)
    {
        //mMouse->mouseDoubleClickEvent(event);
    }
    //SceneWidget::mouseDoubleClickEvent(event);
}

void CSVRender::WorldspaceWidget::wheelEvent (QWheelEvent *event)
{
    //if(!mMouse->wheelEvent(event))
        RenderWidget::wheelEvent(event);
}

void CSVRender::WorldspaceWidget::keyPressEvent (QKeyEvent *event)
{
    if(event->key() == Qt::Key_Escape)
    {
        //mMouse->cancelDrag();
    }
    else
        RenderWidget::keyPressEvent(event);
}
