
#include "worldspacewidget.hpp"

#include <algorithm>

#include <OgreSceneNode.h>
#include <OgreSceneManager.h>
#include <OgreEntity.h>

#include <OgreMeshManager.h>
#include <OgreManualObject.h>        // FIXME: for debugging
#include <OgreMaterialManager.h>     // FIXME: for debugging
#include <OgreHardwarePixelBuffer.h> // FIXME: for debugging

#include <QMouseEvent>
#include <QElapsedTimer>
#include <QtGui/qevent.h>

#include "../../model/world/universalid.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/settings/usersettings.hpp"

#include "../widget/scenetoolmode.hpp"
#include "../widget/scenetooltoggle.hpp"
#include "../widget/scenetoolrun.hpp"

#include "../world/physicssystem.hpp"

#include "elements.hpp"

namespace
{
    // FIXME: this section should be removed once the debugging is completed
    void showHitPoint(Ogre::SceneManager *sceneMgr, std::string name, Ogre::Vector3 point)
    {
        if(sceneMgr->hasManualObject("manual" + name))
            sceneMgr->destroyManualObject("manual" + name);
        Ogre::ManualObject* manual = sceneMgr->createManualObject("manual" + name);
        manual->begin("BaseWhite", Ogre::RenderOperation::OT_LINE_LIST);
        manual-> position(point.x,     point.y,     point.z-100);
        manual-> position(point.x,     point.y,     point.z+100);
        manual-> position(point.x,     point.y-100, point.z);
        manual-> position(point.x,     point.y+100, point.z);
        manual-> position(point.x-100, point.y,     point.z);
        manual-> position(point.x+100, point.y,     point.z);
        manual->end();
        sceneMgr->getRootSceneNode()->createChildSceneNode()->attachObject(manual);
    }

    void removeHitPoint(Ogre::SceneManager *sceneMgr, std::string name)
    {
        if(sceneMgr->hasManualObject("manual" + name))
            sceneMgr->destroyManualObject("manual" + name);
    }

    void initDebug()
    {
        // material for visual cue on selected objects
        Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton().getByName("DynamicTrans");
        if(texture.isNull())
        {
            texture = Ogre::TextureManager::getSingleton().createManual(
                "DynamicTrans", // name
                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                Ogre::TEX_TYPE_2D,  // type
                8, 8,               // width & height
                0,                  // number of mipmaps
                Ogre::PF_BYTE_BGRA, // pixel format
                Ogre::TU_DEFAULT);  // usage; should be TU_DYNAMIC_WRITE_ONLY_DISCARDABLE for
                                    // textures updated very often (e.g. each frame)

            Ogre::HardwarePixelBufferSharedPtr pixelBuffer = texture->getBuffer();
            pixelBuffer->lock(Ogre::HardwareBuffer::HBL_NORMAL);
            const Ogre::PixelBox& pixelBox = pixelBuffer->getCurrentLock();

            uint8_t* pDest = static_cast<uint8_t*>(pixelBox.data);

            // Fill in some pixel data. This will give a semi-transparent colour,
            // but this is of course dependent on the chosen pixel format.
            for (size_t j = 0; j < 8; j++)
            {
                for(size_t i = 0; i < 8; i++)
                {
                    *pDest++ = 255; // B
                    *pDest++ = 255; // G
                    *pDest++ = 127; // R
                    *pDest++ =  63; // A
                }

                pDest += pixelBox.getRowSkip() * Ogre::PixelUtil::getNumElemBytes(pixelBox.format);
            }
            pixelBuffer->unlock();
        }
        Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().getByName(
                    "TransMaterial");
        if(material.isNull())
        {
            Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().create(
                        "TransMaterial",
                        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, true );
            Ogre::Pass *pass = material->getTechnique( 0 )->getPass( 0 );
            pass->setLightingEnabled( false );
            pass->setDepthWriteEnabled( false );
            pass->setSceneBlending( Ogre::SBT_TRANSPARENT_ALPHA );

            Ogre::TextureUnitState *tex = pass->createTextureUnitState("CustomState", 0);
            tex->setTextureName("DynamicTrans");
            tex->setTextureFiltering( Ogre::TFO_ANISOTROPIC );
            material->load();
        }
    }
}

CSVRender::WorldspaceWidget::WorldspaceWidget (CSMDoc::Document& document, QWidget* parent)
: SceneWidget (parent), mDocument(document), mSceneElements(0), mRun(0),
  mCurrentObj(""), mMouseState(Mouse_Default), mOldPos(0,0), mMouseEventTimer(0), mPlane(0),
  mGrabbedSceneNode(""), mOrigObjPos(Ogre::Vector3()), mOrigMousePos(Ogre::Vector3()),
  mCurrentMousePos(Ogre::Vector3()), mZOffset(0.0f)
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

    initDebug();
    mMouseEventTimer = new QElapsedTimer();
    mMouseEventTimer->invalidate();

    mPlane = new Ogre::Plane(Ogre::Vector3::UNIT_Z, 0);
    Ogre::MeshPtr mesh = Ogre::MeshManager::getSingleton().createPlane("ground",
        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
        *mPlane,
        300000,300000, // FIXME: use far clip dist?
        1,1, // segments
        true,  // normals
        1,     // numTexCoordSets
        1,1, // uTile, vTile
        Ogre::Vector3::UNIT_Y // upVector
        );
}

CSVRender::WorldspaceWidget::~WorldspaceWidget ()
{
    delete mMouseEventTimer;

    // For debugging only
    std::map<std::string, std::vector<std::string> >::iterator iter = mSelectedEntities.begin();
    for(;iter != mSelectedEntities.end(); ++iter)
    {
        removeHitPoint(getSceneManager(), iter->first);

        if(getSceneManager()->hasSceneNode(iter->first))
        {
            Ogre::SceneNode *scene = getSceneManager()->getSceneNode(iter->first);

            if(scene)
            {
                scene->removeAndDestroyAllChildren();
                getSceneManager()->destroySceneNode(iter->first);
            }
        }
    }

    delete mPlane;
}

void CSVRender::WorldspaceWidget::selectNavigationMode (const std::string& mode)
{
    if (mode=="1st")
        setNavigation (&m1st);
    else if (mode=="free")
        setNavigation (&mFree);
    else if (mode=="orbit")
        setNavigation (&mOrbit);
}

void CSVRender::WorldspaceWidget::useViewHint (const std::string& hint) {}

void CSVRender::WorldspaceWidget::selectDefaultNavigationMode()
{
    setNavigation (&m1st);
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

CSVWidget::SceneToolToggle *CSVRender::WorldspaceWidget::makeSceneVisibilitySelector (CSVWidget::SceneToolbar *parent)
{
    mSceneElements= new CSVWidget::SceneToolToggle (parent,
        "Scene Element Visibility", ":door.png");

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
        ":door.png", ":faction.png", profiles);

    connect (mRun, SIGNAL (runRequest (const std::string&)),
        this, SLOT (runRequest (const std::string&)));

    return mRun;
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

unsigned int CSVRender::WorldspaceWidget::getElementMask() const
{
    return mSceneElements->getSelection();
}

void CSVRender::WorldspaceWidget::addVisibilitySelectorButtons (
    CSVWidget::SceneToolToggle *tool)
{
    tool->addButton (":activator.png", Element_Reference, ":activator.png", "References");
    tool->addButton (":armor.png", Element_Terrain, ":armor.png", "Terrain");
    tool->addButton (":armor.png", Element_Water, ":armor.png", "Water");
    tool->addButton (":armor.png", Element_Pathgrid, ":armor.png", "Pathgrid");
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
    setVisibilityMask (getElementMask());
    flagAsModified();
    updateOverlay();
}

void CSVRender::WorldspaceWidget::updateOverlay()
{
}

// mouse picking
// FIXME: need to virtualise mouse buttons
//
// State machine:
//
// [default] mousePressEvent->check if the mouse is pointing at an object
//         if yes, create collision planes then go to [grab]
//         else check for terrain
//
// [grab]  mouseReleaseEvent->if same button and new obj, go to [edit]
//         mouseMoveEvent->if same button, go to [drag]
//         other mouse events or buttons, go back to [default] (i.e. like 'cancel')
//
// [drag]  mouseReleaseEvent->if same button, place the object at the new
//         location, update the document then go to [edit]
//         mouseMoveEvent->update position to the user based on ray to the collision
//         planes and render the object at the new location, but do not update
//         the document yet
//
// [edit]  TODO, probably fine positional adjustments or rotations; clone/delete?
//
//
//               press               press (obj)
//   [default] --------> [grab] <-------------------- [edit]
//       ^       (obj)    |  |  ------> [drag] ----->   ^
//       |                |  |   move     ^ |  release  |
//       |                |  |            | |           |
//       |                |  |            +-+           |
//       |                |  |            move          |
//       +----------------+  +--------------------------+
//            release                  release
//           (same obj)               (new obj)
//
//
void CSVRender::WorldspaceWidget::mouseMoveEvent (QMouseEvent *event)
{
    if(event->buttons() & Qt::RightButton)
    {
        switch(mMouseState)
        {
            case Mouse_Grab:
            {
                // check if min elapsed time to stop false detection of drag
                if(!mMouseEventTimer->isValid() || !mMouseEventTimer->hasExpired(100)) // ms
                    break;

                mMouseEventTimer->invalidate();
                mMouseState = Mouse_Drag;

                /* FALL_THROUGH */
            }
            case Mouse_Drag:
            {
                if(event->pos() != mOldPos) // TODO: maybe don't update less than a quantum?
                {
                    mOldPos = event->pos();

                    // ray test against the plane to provide feedback to the user the
                    // relative movement of the object on the x-y plane
                    std::pair<bool, Ogre::Vector3> planeResult = mousePositionOnPlane(event->pos(), *mPlane);
                    if(planeResult.first)
                    {
                        if(mGrabbedSceneNode != "")
                        {
                            Ogre::Vector3 pos = mOrigObjPos;
                            pos.z += mZOffset;
                            getSceneManager()->getSceneNode(mGrabbedSceneNode)->setPosition(pos+planeResult.second-mOrigMousePos);
                            mCurrentMousePos = planeResult.second;
                            CSVWorld::PhysicsSystem::instance()->moveSceneNodes(mGrabbedSceneNode,
                                pos+planeResult.second-mOrigMousePos);
                            emit signalAsModified();
                        }
                    }
                }
                break;
            }
            case Mouse_Edit:
            case Mouse_Default:
            {
                break; // error event, ignore
            }
            /* NO_DEFAULT_CASE */
        }
    }
    SceneWidget::mouseMoveEvent(event);
}

void CSVRender::WorldspaceWidget::mousePressEvent (QMouseEvent *event)
{
    switch(mMouseState)
    {
        case Mouse_Grab:
        case Mouse_Drag:
        {
            if(0 /*event->buttons() & ~Qt::RightButton*/)
            {
                // cancel operation & return the object to the original position
                placeObject(mGrabbedSceneNode, mOrigObjPos);
                mMouseState = Mouse_Default;

                // reset states
                mCurrentMousePos = Ogre::Vector3();
                mOrigMousePos = Ogre::Vector3();
                mOrigObjPos = Ogre::Vector3();
                mGrabbedSceneNode = "";
                mCurrentObj = "";
                mOldPos = QPoint(0, 0);
                mMouseEventTimer->invalidate();
                mZOffset = 0.0f;
            }
            break;
        }
        case Mouse_Edit:
        case Mouse_Default:
        {
            if(event->buttons() & Qt::RightButton)
            {
                std::pair<std::string, Ogre::Vector3> result = objectUnderCursor(event->x(), event->y());
                if(result.first == "")
                    break;

                mGrabbedSceneNode = result.first;
                // ray test agaist the plane to get a starting position of the
                // mouse in relation to the object position
                mPlane->redefine(Ogre::Vector3::UNIT_Z, result.second);
                std::pair<bool, Ogre::Vector3> planeResult = mousePositionOnPlane(event->pos(), *mPlane);
                if(planeResult.first)
                {
                    mOrigMousePos = planeResult.second;
                    mCurrentMousePos = planeResult.second;
                    mZOffset = 0.0f;
                }

                mOrigObjPos = getSceneManager()->getSceneNode(mGrabbedSceneNode)->getPosition();
                mMouseEventTimer->start();

                mMouseState = Mouse_Grab;
            }
            break;
        }
        /* NO_DEFAULT_CASE */
    }
    //SceneWidget::mousePressEvent(event);
}

void CSVRender::WorldspaceWidget::mouseReleaseEvent (QMouseEvent *event)
{
    if(event->button() == Qt::RightButton)
    {
        if(!getCamera()->getViewport())
        {
            SceneWidget::mouseReleaseEvent(event);
            return;
        }

        switch(mMouseState)
        {
            case Mouse_Grab:
            {
                std::pair<std::string, Ogre::Vector3> result = objectUnderCursor(event->x(), event->y());
                if(result.first != "")
                {
                    if(result.first == mCurrentObj)
                    {
                        // unselect object
                        mMouseState = Mouse_Default;
                        mCurrentObj = "";
                    }
                    else
                    {
                        // select object
                        mMouseState = Mouse_Edit;
                        mCurrentObj = result.first;

                        // print some debug info
                        if(isDebug())
                        {
                            std::string referenceId =
                                CSVWorld::PhysicsSystem::instance()->sceneNodeToRefId(result.first);
                            std::cout << "ReferenceId: " << referenceId << std::endl;
                            const CSMWorld::RefCollection& references = mDocument.getData().getReferences();
                            int index = references.searchId(referenceId);
                            if (index != -1)
                            {
                                int columnIndex =
                                    references.findColumnIndex(CSMWorld::Columns::ColumnId_ReferenceableId);
                                std::cout << "  index: " + QString::number(index).toStdString()
                                          +", column index: " + QString::number(columnIndex).toStdString()
                                          << std::endl;
                            }
                        }
                    }
                    // update highlighting the current object
                    if(isDebug())
                        updateSelectionHighlight(result.first, result.second);
                }
                break;
            }
            case Mouse_Drag:
            {
                // final placement
                std::pair<bool, Ogre::Vector3> planeResult = mousePositionOnPlane(event->pos(), *mPlane);
                if(planeResult.first)
                {
                    if(mGrabbedSceneNode != "")
                    {
                        mOrigObjPos.z += mZOffset;
                        Ogre::Vector3 pos = mOrigObjPos+planeResult.second-mOrigMousePos;
                        placeObject(mGrabbedSceneNode, pos);
                        //mCurrentObj = mGrabbedSceneNode; // FIXME
                        mCurrentObj = "";                   // whether the object is selected

                        // reset states
                        mCurrentMousePos = Ogre::Vector3(); // mouse pos to use in wheel event
                        mOrigMousePos = Ogre::Vector3();    // starting pos of mouse in world space
                        mOrigObjPos = Ogre::Vector3();      // starting pos of object in world space
                        mGrabbedSceneNode = "";             // id of the object
                        mZOffset = 0.0f;                    // used for z-axis movement
                        mOldPos = QPoint(0, 0);             // to calculate relative movement of mouse
                                                            //  on screen

                        // FIXME: update document
                        // FIXME: highlight current object?
                        mMouseState = Mouse_Edit;
                    }
                }
                break;
            }
            case Mouse_Edit:
            case Mouse_Default:
            {
                // probably terrain, check
                std::pair<std::string, Ogre::Vector3> result = terrainUnderCursor(event->x(), event->y());
                if(result.first != "")
                {
                    if(isDebug())
                    {
                        std::cout << "terrain: " << result.first << std::endl;
                        std::cout << "  hit pos "+ QString::number(result.second.x).toStdString()
                                + ", " + QString::number(result.second.y).toStdString()
                                + ", " + QString::number(result.second.z).toStdString() << std::endl;
                    }
                }
                break;
            }
            /* NO_DEFAULT_CASE */
        }
        mMouseEventTimer->invalidate();
    }
    SceneWidget::mouseReleaseEvent(event);
}

void CSVRender::WorldspaceWidget::mouseDoubleClickEvent (QMouseEvent *event)
{
    if(event->button() == Qt::RightButton)
    {
        if(isDebug())
        {
            // FIXME: OEngine::PhysicEngine creates only one child scene node for the
            // debug drawer.  Hence only the first subview that creates the debug drawer
            // can view the debug lines.  Will need to keep a map in OEngine if multiple
            // subviews are to be supported.
            //CSVWorld::PhysicsSystem::instance()->setSceneManager(getSceneManager());
            CSVWorld::PhysicsSystem::instance()->toggleDebugRendering(getSceneManager());
            flagAsModified();
        }
    }
    //SceneWidget::mouseDoubleClickEvent(event);
}

void CSVRender::WorldspaceWidget::wheelEvent (QWheelEvent *event)
{
    switch(mMouseState)
    {
        case Mouse_Grab:
            mMouseState = Mouse_Drag;

            /* FALL_THROUGH */
        case Mouse_Drag:
        {
            // move the object along the z axis during Mouse_Drag or Mouse_Grab
            if (event->delta())
            {
                // seems positive is up and negative is down
                mZOffset += (event->delta()/5); // FIXME: arbitrary number, make config option?

                Ogre::Vector3 pos = mOrigObjPos;
                pos.z += mZOffset;
                getSceneManager()->getSceneNode(mGrabbedSceneNode)->setPosition(pos+mCurrentMousePos-mOrigMousePos);
                CSVWorld::PhysicsSystem::instance()->moveSceneNodes(mGrabbedSceneNode,
                    pos+mCurrentMousePos-mOrigMousePos);
                emit signalAsModified();
            }
            break;
        }
        case Mouse_Edit:
        case Mouse_Default:
        {
            SceneWidget::wheelEvent(event);
            break;
        }
        /* NO_DEFAULT_CASE */
    }
}

// FIXME: for debugging only
void CSVRender::WorldspaceWidget::updateSelectionHighlight(const std::string sceneNode, const Ogre::Vector3 &position)
{
    uint32_t visibilityMask = getCamera()->getViewport()->getVisibilityMask();
    bool ignoreObjects = !(visibilityMask & (uint32_t)CSVRender::Element_Reference);

    if(ignoreObjects || !getSceneManager()->hasSceneNode(sceneNode) || !isDebug())
        return;

    CSMSettings::UserSettings &userSettings = CSMSettings::UserSettings::instance();
    bool debugCursor = userSettings.setting(
                "debug/mouse-position", QString("false")) == "true" ? true : false;

    //TODO: Try http://www.ogre3d.org/tikiwiki/Create+outline+around+a+character
    Ogre::SceneNode *scene = getSceneManager()->getSceneNode(sceneNode);
    std::map<std::string, std::vector<std::string> >::iterator iter =
                                            mSelectedEntities.find(sceneNode);
    if(iter != mSelectedEntities.end()) // currently selected
    {
        std::vector<std::string> clonedEntities = mSelectedEntities[sceneNode];
        while(!clonedEntities.empty())
        {
            if(getSceneManager()->hasEntity(clonedEntities.back()))
            {
                scene->detachObject(clonedEntities.back());
                getSceneManager()->destroyEntity(clonedEntities.back());
            }
            clonedEntities.pop_back();
        }
        mSelectedEntities.erase(iter);

        if(debugCursor)
            removeHitPoint(getSceneManager(), sceneNode);
    }
    else
    {
        std::vector<std::string> clonedEntities;
        Ogre::SceneNode::ObjectIterator iter = scene->getAttachedObjectIterator();
        iter.begin();
        while(iter.hasMoreElements())
        {
            Ogre::MovableObject * element = iter.getNext();
            if(!element)
                break;

            if(element->getMovableType() != "Entity")
                continue;

            Ogre::Entity * entity = dynamic_cast<Ogre::Entity *>(element);
            if(getSceneManager()->hasEntity(entity->getName()+"cover"))
            {
                // FIXME: this shouldn't really happen... but does :(
                scene->detachObject(entity->getName()+"cover");
                getSceneManager()->destroyEntity(entity->getName()+"cover");
            }
            Ogre::Entity * clone = entity->clone(entity->getName()+"cover");

            Ogre::MaterialPtr mat =
                Ogre::MaterialManager::getSingleton().getByName("TransMaterial");
            if(!mat.isNull())
            {
                clone->setMaterial(mat);
                scene->attachObject(clone);
                clonedEntities.push_back(entity->getName()+"cover");
            }
        }
        mSelectedEntities[sceneNode] = clonedEntities;

        if(debugCursor)
            showHitPoint(getSceneManager(), sceneNode, position);
    }
    flagAsModified();
}

std::pair<std::string, Ogre::Vector3> CSVRender::WorldspaceWidget::terrainUnderCursor(const int mouseX, const int mouseY)
{
    if(!getCamera()->getViewport())
        return std::make_pair("", Ogre::Vector3());

    float x = (float) mouseX / getCamera()->getViewport()->getActualWidth();
    float y = (float) mouseY / getCamera()->getViewport()->getActualHeight();

    std::pair<std::string, Ogre::Vector3> result =
        CSVWorld::PhysicsSystem::instance()->castRay(x, y, getSceneManager(), getCamera());
    if(result.first != "")
    {
        // FIXME: is there  a better way to distinguish terrain from objects?
        QString name  = QString(result.first.c_str());
        if(name.contains(QRegExp("^HeightField")))
        {
            return result;
        }
    }

    return std::make_pair("", Ogre::Vector3());
}

std::pair<std::string, Ogre::Vector3> CSVRender::WorldspaceWidget::objectUnderCursor(const int mouseX, const int mouseY)
{
    if(!getCamera()->getViewport())
        return std::make_pair("", Ogre::Vector3());

    float x = (float) mouseX / getCamera()->getViewport()->getActualWidth();
    float y = (float) mouseY / getCamera()->getViewport()->getActualHeight();

    std::pair<std::string, Ogre::Vector3> result =
        CSVWorld::PhysicsSystem::instance()->castRay(x, y, getSceneManager(), getCamera());
    if(result.first != "")
    {
        // NOTE: anything not terrain is assumed to be an object
        QString name  = QString(result.first.c_str());
        if(!name.contains(QRegExp("^HeightField")))
        {
            uint32_t visibilityMask = getCamera()->getViewport()->getVisibilityMask();
            bool ignoreObjects = !(visibilityMask & (uint32_t)CSVRender::Element_Reference);

            if(!ignoreObjects && getSceneManager()->hasSceneNode(result.first))
            {
                return result;
            }
        }
    }

    return std::make_pair("", Ogre::Vector3());
}

std::pair<bool, Ogre::Vector3> CSVRender::WorldspaceWidget::mousePositionOnPlane(const QPoint &pos, const Ogre::Plane &plane)
{
    // using a really small value seems to mess up with the projections
    float nearClipDistance = getCamera()->getNearClipDistance(); // save existing
    getCamera()->setNearClipDistance(10.0f);  // arbitrary number
    Ogre::Ray mouseRay = getCamera()->getCameraToViewportRay(
        (float) pos.x() / getCamera()->getViewport()->getActualWidth(),
        (float) pos.y() / getCamera()->getViewport()->getActualHeight());
    getCamera()->setNearClipDistance(nearClipDistance); // restore
    std::pair<bool, float> planeResult = mouseRay.intersects(plane);

    if(planeResult.first)
        return std::make_pair(true, mouseRay.getPoint(planeResult.second));
    else
        return std::make_pair(false, Ogre::Vector3()); // should only happen if the plane is too small
}

void CSVRender::WorldspaceWidget::placeObject(const std::string sceneNode, const Ogre::Vector3 &pos)
{
    getSceneManager()->getSceneNode(sceneNode)->setPosition(pos);

    // update physics
    std::string refId = CSVWorld::PhysicsSystem::instance()->sceneNodeToRefId(sceneNode);
    const CSMWorld::CellRef& cellref =
            mDocument.getData().getReferences().getRecord (refId).get();
    Ogre::Quaternion xr (Ogre::Radian (-cellref.mPos.rot[0]), Ogre::Vector3::UNIT_X);
    Ogre::Quaternion yr (Ogre::Radian (-cellref.mPos.rot[1]), Ogre::Vector3::UNIT_Y);
    Ogre::Quaternion zr (Ogre::Radian (-cellref.mPos.rot[2]), Ogre::Vector3::UNIT_Z);

    // FIXME: adjustRigidBody() seems to lose objects, work around by deleting and recreating objects
    //CSVWorld::PhysicsSystem::instance()->moveObject(sceneNode, pos, xr*yr*zr);
    std::string mesh = CSVWorld::PhysicsSystem::instance()->sceneNodeToMesh(sceneNode);
    CSVWorld::PhysicsSystem::instance()->replaceObject(mesh, sceneNode, refId, cellref.mScale, pos, xr*yr*zr);

    // update all SceneWidgets and their SceneManagers
    emit signalAsModified();
}

bool CSVRender::WorldspaceWidget::isDebug()
{
    return false;
}
