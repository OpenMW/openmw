
#include "pagedworldspacewidget.hpp"

#include <sstream>

#include <QMouseEvent>
#include <QElapsedTimer>

#include <OgreCamera.h>
#include <OgreSceneManager.h>
#include <OgreManualObject.h>
#include <OgreOverlayContainer.h>
#include <OgreOverlayManager.h>
#include <OgreRoot.h>
#include <OgreSceneQuery.h>

#include <OgreEntity.h>              // FIXME: visual highlight, clone
#include <OgreMaterialManager.h>     // FIXME: visual highlight, material
#include <OgreHardwarePixelBuffer.h> // FIXME: visual highlight, texture

#include <OgreMeshManager.h>

#include <components/esm/loadland.hpp>
#include "textoverlay.hpp"
#include "overlaymask.hpp"

#include "../../model/world/tablemimedata.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/settings/usersettings.hpp"

#include "../widget/scenetooltoggle.hpp"
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

bool CSVRender::PagedWorldspaceWidget::adjustCells()
{
    bool modified = false;
    bool setCamera = false;

    const CSMWorld::IdCollection<CSMWorld::Cell>& cells = mDocument.getData().getCells();

    {
        // remove (or name/region modified)
        std::map<CSMWorld::CellCoordinates, Cell *>::iterator iter (mCells.begin());

        while (iter!=mCells.end())
        {
            int index = cells.searchId (iter->first.getId (mWorldspace));

            if (!mSelection.has (iter->first) || index==-1 ||
                cells.getRecord (index).mState==CSMWorld::RecordBase::State_Deleted)
            {
                // delete overlays
                std::map<CSMWorld::CellCoordinates, TextOverlay *>::iterator itOverlay = mTextOverlays.find(iter->first);
                if(itOverlay != mTextOverlays.end())
                {
                    delete itOverlay->second;
                    mTextOverlays.erase(itOverlay);
                }

                // destroy manual objects
                getSceneManager()->destroyManualObject("manual"+iter->first.getId(mWorldspace));

                delete iter->second;
                mCells.erase (iter++);

                modified = true;
            }
            else
            {
                // check if name or region field has changed
                // FIXME: config setting
                std::string name = cells.getRecord(index).get().mName;
                std::string region = cells.getRecord(index).get().mRegion;

                std::map<CSMWorld::CellCoordinates, TextOverlay *>::iterator it = mTextOverlays.find(iter->first);
                if(it != mTextOverlays.end())
                {
                    if(it->second->getDesc() != "") // previously had name
                    {
                        if(name != it->second->getDesc()) // new name
                        {
                            if(name != "")
                                it->second->setDesc(name);
                            else // name deleted, use region
                                it->second->setDesc(region);
                            it->second->update();
                        }
                    }
                    else if(name != "") // name added
                    {
                        it->second->setDesc(name);
                        it->second->update();
                    }
                    else if(region != it->second->getDesc()) // new region
                    {
                        it->second->setDesc(region);
                        it->second->update();
                    }
                    modified = true;
                }
                ++iter;
            }
        }
    }

    if (mCells.begin()==mCells.end())
        setCamera = true;

    // add
    for (CSMWorld::CellSelection::Iterator iter (mSelection.begin()); iter!=mSelection.end();
        ++iter)
    {
        int index = cells.searchId (iter->getId (mWorldspace));

        if (index > 0 && cells.getRecord (index).mState!=CSMWorld::RecordBase::State_Deleted &&
            mCells.find (*iter)==mCells.end())
        {
            Cell *cell = new Cell (mDocument.getData(), getSceneManager(),
                    iter->getId (mWorldspace));
            mCells.insert (std::make_pair (*iter, cell));

            float height = cell->getTerrainHeightAt(Ogre::Vector3(
                              ESM::Land::REAL_SIZE * iter->getX() + ESM::Land::REAL_SIZE/2,
                              ESM::Land::REAL_SIZE * iter->getY() + ESM::Land::REAL_SIZE/2,
                              0));
            if (setCamera)
            {
                setCamera = false;
                getCamera()->setPosition (
                              ESM::Land::REAL_SIZE * iter->getX() + ESM::Land::REAL_SIZE/2,
                              ESM::Land::REAL_SIZE * iter->getY() + ESM::Land::REAL_SIZE/2,
                              height);
                // better camera position at the start
                getCamera()->move(getCamera()->getDirection() * -6000); // FIXME: config setting
            }

            Ogre::ManualObject* manual =
                    getSceneManager()->createManualObject("manual" + iter->getId(mWorldspace));
            manual->begin("BaseWhite", Ogre::RenderOperation::OT_LINE_LIST);
            // define start and end point (x, y, z)
            manual-> position(ESM::Land::REAL_SIZE * iter->getX() + ESM::Land::REAL_SIZE/2,
                              ESM::Land::REAL_SIZE * iter->getY() + ESM::Land::REAL_SIZE/2,
                              height);
            manual-> position(ESM::Land::REAL_SIZE * iter->getX() + ESM::Land::REAL_SIZE/2,
                              ESM::Land::REAL_SIZE * iter->getY() + ESM::Land::REAL_SIZE/2,
                              height+200); // FIXME: config setting
            manual->end();
            manual->setBoundingBox(Ogre::AxisAlignedBox(
                              ESM::Land::REAL_SIZE * iter->getX() + ESM::Land::REAL_SIZE/2,
                              ESM::Land::REAL_SIZE * iter->getY() + ESM::Land::REAL_SIZE/2,
                              height,
                              ESM::Land::REAL_SIZE * iter->getX() + ESM::Land::REAL_SIZE/2,
                              ESM::Land::REAL_SIZE * iter->getY() + ESM::Land::REAL_SIZE/2,
                              height+200));
            getSceneManager()->getRootSceneNode()->createChildSceneNode()->attachObject(manual);
            manual->setVisible(false);

            CSVRender::TextOverlay *textDisp =
                    new CSVRender::TextOverlay(manual, getCamera(), iter->getId(mWorldspace));
            textDisp->enable(true);
            textDisp->setCaption(iter->getId(mWorldspace));
            std::string desc = cells.getRecord(index).get().mName;
            if(desc == "") desc = cells.getRecord(index).get().mRegion;
            textDisp->setDesc(desc); // FIXME: config setting
            textDisp->update();
            mTextOverlays.insert(std::make_pair(*iter, textDisp));
            if(!mOverlayMask)
            {
                mOverlayMask = new OverlayMask(mTextOverlays, getViewport());
                addRenderTargetListener(mOverlayMask);
            }

            modified = true;
        }
    }

    return modified;
}

// mouse picking
// FIXME: need to virtualise mouse buttons
//
// State machine:
//
// [default] mousePressEvent->check if the mouse is pointing at an object
//         if yes, go to [grab] else stay at [default]
//
// [grab] mouseReleaseEvent->if same button and new obj, go to [edit]
//         mouseMoveEvent->if same button, create collision planes then go to [drag]
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
void CSVRender::PagedWorldspaceWidget::mouseMoveEvent (QMouseEvent *event)
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
                else
                {
                    mMouseEventTimer->invalidate();

                    mMouseState = Mouse_Drag;
                    //std::cout << "grab->drag" << std::endl;
                }

                /* FALL_THROUGH */
            }
            case Mouse_Drag:
            {
                // FIXME: don't update less than a quantum
                //QPoint diff = mOldPos-event->pos();
                if(event->pos() != mOldPos)
                {
                    mOldPos = event->pos();
                    //std::cout << QString::number(event->pos().x()).toStdString() << ", "
                              //<< QString::number(event->pos().y()).toStdString() << std::endl;

                    // ray test against the plane to provide feedback to the user the
                    // relative movement of the object on the x-y plane
                    std::pair<bool, Ogre::Vector3> planeResult = mousePositionOnPlane(event, *mPlane);
                    if(planeResult.first)
                    {
                        if(mObjSceneNode)
                        {
                            mObjSceneNode->setPosition(mOrigObjPos+planeResult.second-mOrigMousePos);
                            flagAsModified();
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

void CSVRender::PagedWorldspaceWidget::mousePressEvent (QMouseEvent *event)
{
    if(event->buttons() & Qt::RightButton)
    {
        switch(mMouseState)
        {
            case Mouse_Grab:
            case Mouse_Drag:
            {
                break; // error event, ignore
            }
            case Mouse_Edit:
            case Mouse_Default:
            {
                if(!getCamera()->getViewport())
                    break;

                std::pair<std::string, Ogre::Vector3> result = isObjectUnderCursor(
                    (float) event->x() / getCamera()->getViewport()->getActualWidth(),
                    (float) event->y() / getCamera()->getViewport()->getActualHeight());
                if(result.first != "")
                {
                    mCurrentObj = result.first; // FIXME
                    // ray test agaist the plane to get a starting position of the
                    // mouse in relation to the object position
                    mPlane->redefine(Ogre::Vector3::UNIT_Z, result.second);
                    std::pair<bool, Ogre::Vector3> planeResult = mousePositionOnPlane(event, *mPlane);
                    if(planeResult.first)
                        mOrigMousePos = planeResult.second;

                    std::string sceneNodeName =
                        CSVWorld::PhysicsSystem::instance()->referenceToSceneNode(result.first);
                    mObjSceneNode = getSceneManager()->getSceneNode(sceneNodeName);
                    mOrigObjPos = mObjSceneNode->getPosition();


                    mMouseEventTimer->start();

                    mMouseState = Mouse_Grab;
                    //std::cout << "default/edit->grab" << std::endl;
                }
                break;
            }
            /* NO_DEFAULT_CASE */
        }
    }
    // FIXME: other button press - cancel grab and/or drag and place the object back in the original
    // position
    //SceneWidget::mousePressEvent(event);
}

void CSVRender::PagedWorldspaceWidget::mouseReleaseEvent (QMouseEvent *event)
{
    if(event->button() == Qt::RightButton)
    {
        std::map<CSMWorld::CellCoordinates, TextOverlay *>::iterator iter = mTextOverlays.begin();
        for(; iter != mTextOverlays.end(); ++iter)
        {
            if(mDisplayCellCoord &&
               iter->second->isEnabled() && iter->second->container().contains(event->x(), event->y()))
            {
                std::cout << "clicked: " << iter->second->getCaption() << std::endl;
                break;
            }
        }

        if(!getCamera()->getViewport())
        {
            SceneWidget::mouseReleaseEvent(event);
            return;
        }

        // FIXME: skip this if overlay clicked above
        // FIXME: stop/disable the timer
        switch(mMouseState)
        {
            case Mouse_Grab:
            {
                std::pair<std::string, Ogre::Vector3> result = isObjectUnderCursor(
                    (float) event->x() / getCamera()->getViewport()->getActualWidth(),
                    (float) event->y() / getCamera()->getViewport()->getActualHeight());
                if(result.first != "")
                {
                    if(result.first == mCurrentObj)
                    {
                        mMouseState = Mouse_Default;
                        //std::cout << "grab->default" << std::endl;
                        mCurrentObj = "";
                    }
                    else
                    {
                        mMouseState = Mouse_Edit;
                        //std::cout << "grab->edit" << std::endl;
                        mCurrentObj = result.first;


                        // print some debug info
                        std::cout << "ReferenceId: " << result.first << std::endl;
                        const CSMWorld::RefCollection& references = mDocument.getData().getReferences();
                        int index = references.searchId(result.first);
                        if (index != -1)
                        {
                            int columnIndex =
                                references.findColumnIndex(CSMWorld::Columns::ColumnId_ReferenceableId);
                            std::cout << "  index: " + QString::number(index).toStdString()
                                      +", column index: " + QString::number(columnIndex).toStdString()
                                      << std::endl;
                        }
                    }
                    // update highlighting the current object
                    std::string sceneNode =
                        CSVWorld::PhysicsSystem::instance()->referenceToSceneNode(result.first);

                    uint32_t visibilityMask = getCamera()->getViewport()->getVisibilityMask();
                    bool ignoreObjects = !(visibilityMask & (uint32_t)CSVRender::Element_Reference);

                    if(!ignoreObjects && getSceneManager()->hasSceneNode(sceneNode))
                    {
                        CSMSettings::UserSettings &userSettings = CSMSettings::UserSettings::instance();
                        if(userSettings.setting("debug/mouse-picking", QString("false")) == "true" ? true : false)
                            updateSelectionHighlight(sceneNode, result.second);
                    }
                    flagAsModified();
                }
                break;
            }
            case Mouse_Drag:
            {
                // final placement
                std::pair<bool, Ogre::Vector3> planeResult = mousePositionOnPlane(event, *mPlane);
                if(planeResult.first)
                {
                    if(mObjSceneNode)
                    {
                        mObjSceneNode->setPosition(mOrigObjPos+planeResult.second-mOrigMousePos);
                        flagAsModified();

                        // update physics
                        const CSMWorld::CellRef& cellref =
                                mDocument.getData().getReferences().getRecord (mCurrentObj).get();
                        Ogre::Quaternion xr (Ogre::Radian (-cellref.mPos.rot[0]), Ogre::Vector3::UNIT_X);
                        Ogre::Quaternion yr (Ogre::Radian (-cellref.mPos.rot[1]), Ogre::Vector3::UNIT_Y);
                        Ogre::Quaternion zr (Ogre::Radian (-cellref.mPos.rot[2]), Ogre::Vector3::UNIT_Z);

                        // FIXME: adjustRigidBody() seems to lose objects, delete and recreate for now
                        //CSVWorld::PhysicsSystem::instance()->moveObject(mCurrentObj,
                                //mOrigObjPos+planeResult.second-mOrigMousePos, xr*yr*zr);
                        std::string sceneNodeName =
                            CSVWorld::PhysicsSystem::instance()->referenceToSceneNode(mCurrentObj);
                        std::string mesh =
                            CSVWorld::PhysicsSystem::instance()->sceneNodeToMesh(sceneNodeName);
                        CSVWorld::PhysicsSystem::instance()->removeObject(mCurrentObj);
                        CSVWorld::PhysicsSystem::instance()->addObject(mesh,
                            sceneNodeName, mCurrentObj, cellref.mScale,
                            mOrigObjPos+planeResult.second-mOrigMousePos, xr*yr*zr);
                    }
                }
                // FIXME: update document
                // FIXME: highlight current object?
                //std::cout << "final position" << std::endl;

                mMouseState = Mouse_Edit;
                //std::cout << "drag->edit" << std::endl;
                break;
            }
            case Mouse_Edit:
            case Mouse_Default:
            {
                // probably terrain
                debugMousePicking(
                    (float) event->x() / getCamera()->getViewport()->getActualWidth(),
                    (float) event->y() / getCamera()->getViewport()->getActualHeight());
                break;
            }
            /* NO_DEFAULT_CASE */
        }
    }
    SceneWidget::mouseReleaseEvent(event);
}

void CSVRender::PagedWorldspaceWidget::mouseDoubleClickEvent (QMouseEvent *event)
{
    if(event->button() == Qt::RightButton)
    {
        std::cout << "double clicked" << std::endl;

        CSMSettings::UserSettings &userSettings = CSMSettings::UserSettings::instance();
        if(userSettings.setting ("debug/mouse-picking", QString("false")) == "true" ? true : false)
        {
            // FIXME: OEngine::PhysicEngine creates only one child scene node for the
            // debug drawer.  Hence only the first subview that creates the debug drawer
            // can view the debug lines.  Will need to keep a map in OEngine if multiple
            // subviews are to be supported.
            CSVWorld::PhysicsSystem::instance()->setSceneManager(getSceneManager());
            CSVWorld::PhysicsSystem::instance()->toggleDebugRendering();
            flagAsModified();
        }
    }
    //SceneWidget::mouseDoubleClickEvent(event);
}

void CSVRender::PagedWorldspaceWidget::wheelEvent (QWheelEvent *event)
{
    // FIXME: add wheel event to move the object along the y axis during Mouse_Drag or
    // Mouse_Grab
    SceneWidget::wheelEvent(event);
}

void CSVRender::PagedWorldspaceWidget::updateOverlay()
{
    if(getCamera()->getViewport())
    {
        if((uint32_t)getCamera()->getViewport()->getVisibilityMask()
                                & (uint32_t)CSVRender::Element_CellMarker)
            mDisplayCellCoord = true;
        else
            mDisplayCellCoord = false;
    }

    if(!mTextOverlays.empty())
    {
        std::map<CSMWorld::CellCoordinates, TextOverlay *>::iterator it = mTextOverlays.begin();
        for(; it != mTextOverlays.end(); ++it)
        {
            it->second->enable(mDisplayCellCoord);
            it->second->update();
        }
    }
}

void CSVRender::PagedWorldspaceWidget::referenceableDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    for (std::map<CSMWorld::CellCoordinates, Cell *>::iterator iter (mCells.begin());
        iter!=mCells.end(); ++iter)
        if (iter->second->referenceableDataChanged (topLeft, bottomRight))
            flagAsModified();
}

void CSVRender::PagedWorldspaceWidget::referenceableAboutToBeRemoved (
    const QModelIndex& parent, int start, int end)
{
    for (std::map<CSMWorld::CellCoordinates, Cell *>::iterator iter (mCells.begin());
        iter!=mCells.end(); ++iter)
        if (iter->second->referenceableAboutToBeRemoved (parent, start, end))
            flagAsModified();
}

void CSVRender::PagedWorldspaceWidget::referenceableAdded (const QModelIndex& parent,
    int start, int end)
{
    CSMWorld::IdTable& referenceables = dynamic_cast<CSMWorld::IdTable&> (
        *mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_Referenceables));

    for (std::map<CSMWorld::CellCoordinates, Cell *>::iterator iter (mCells.begin());
        iter!=mCells.end(); ++iter)
    {
        QModelIndex topLeft = referenceables.index (start, 0);
        QModelIndex bottomRight =
            referenceables.index (end, referenceables.columnCount());

        if (iter->second->referenceableDataChanged (topLeft, bottomRight))
            flagAsModified();
    }
}

void CSVRender::PagedWorldspaceWidget::referenceDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    for (std::map<CSMWorld::CellCoordinates, Cell *>::iterator iter (mCells.begin());
        iter!=mCells.end(); ++iter)
        if (iter->second->referenceDataChanged (topLeft, bottomRight))
            flagAsModified();
}

void CSVRender::PagedWorldspaceWidget::referenceAboutToBeRemoved (const QModelIndex& parent,
    int start, int end)
{
    for (std::map<CSMWorld::CellCoordinates, Cell *>::iterator iter (mCells.begin());
        iter!=mCells.end(); ++iter)
        if (iter->second->referenceAboutToBeRemoved (parent, start, end))
            flagAsModified();
}

void CSVRender::PagedWorldspaceWidget::referenceAdded (const QModelIndex& parent, int start,
    int end)
{
    for (std::map<CSMWorld::CellCoordinates, Cell *>::iterator iter (mCells.begin());
        iter!=mCells.end(); ++iter)
        if (iter->second->referenceAdded (parent, start, end))
            flagAsModified();
}

std::string CSVRender::PagedWorldspaceWidget::getStartupInstruction()
{
    Ogre::Vector3 position = getCamera()->getPosition();

    std::ostringstream stream;

    stream
        << "player->position "
        << position.x << ", " << position.y << ", " << position.z
        << ", 0";

    return stream.str();
}

CSVRender::PagedWorldspaceWidget::PagedWorldspaceWidget (QWidget* parent, CSMDoc::Document& document)
: WorldspaceWidget (document, parent), mDocument (document), mWorldspace ("std::default"),
  mControlElements(NULL), mDisplayCellCoord(true), mOverlayMask(NULL),
  mCurrentObj(""), mMouseState(Mouse_Default), mOldPos(0,0), mMouseEventTimer(0), mPlane(0)
{
    QAbstractItemModel *cells =
        document.getData().getTableModel (CSMWorld::UniversalId::Type_Cells);

    connect (cells, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
        this, SLOT (cellDataChanged (const QModelIndex&, const QModelIndex&)));
    connect (cells, SIGNAL (rowsRemoved (const QModelIndex&, int, int)),
        this, SLOT (cellRemoved (const QModelIndex&, int, int)));
    connect (cells, SIGNAL (rowsInserted (const QModelIndex&, int, int)),
        this, SLOT (cellAdded (const QModelIndex&, int, int)));

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

CSVRender::PagedWorldspaceWidget::~PagedWorldspaceWidget()
{
    for (std::map<CSMWorld::CellCoordinates, Cell *>::iterator iter (mCells.begin());
        iter!=mCells.end(); ++iter)
    {
        delete iter->second;

        getSceneManager()->destroyManualObject("manual"+iter->first.getId(mWorldspace));
    }

    for (std::map<CSMWorld::CellCoordinates, TextOverlay *>::iterator iter (mTextOverlays.begin());
        iter != mTextOverlays.end(); ++iter)
    {
        delete iter->second;
    }

    removeRenderTargetListener(mOverlayMask);
    delete mOverlayMask;

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

void CSVRender::PagedWorldspaceWidget::useViewHint (const std::string& hint)
{
    if (!hint.empty())
    {
        CSMWorld::CellSelection selection;

        if (hint[0]=='c')
        {
            // syntax: c:#x1 y1; #x2 y2 (number of coordinate pairs can be 0 or larger)
            char ignore;

            std::istringstream stream (hint.c_str());
            if (stream >> ignore)
            {
                char ignore1; // : or ;
                char ignore2; // #
                int x, y;

                while (stream >> ignore1 >> ignore2 >> x >> y)
                    selection.add (CSMWorld::CellCoordinates (x, y));

                /// \todo adjust camera position
            }
        }
        else if (hint[0]=='r')
        {
            /// \todo implement 'r' type hints
        }

        setCellSelection (selection);
    }
}

void CSVRender::PagedWorldspaceWidget::setCellSelection (const CSMWorld::CellSelection& selection)
{
    mSelection = selection;

    if (adjustCells())
        flagAsModified();

    emit cellSelectionChanged (mSelection);
}

std::pair< int, int > CSVRender::PagedWorldspaceWidget::getCoordinatesFromId (const std::string& record) const
{
    std::istringstream stream (record.c_str());
    char ignore;
    int x, y;
    stream >> ignore >> x >> y;
    return std::make_pair(x, y);
}

bool CSVRender::PagedWorldspaceWidget::handleDrop (
    const std::vector< CSMWorld::UniversalId >& data, DropType type)
{
    if (WorldspaceWidget::handleDrop (data, type))
        return true;

    if (type!=Type_CellsExterior)
        return false;

    bool selectionChanged = false;
    for (unsigned i = 0; i < data.size(); ++i)
    {
        std::pair<int, int> coordinates(getCoordinatesFromId(data[i].getId()));
        if (mSelection.add(CSMWorld::CellCoordinates(coordinates.first, coordinates.second)))
        {
            selectionChanged = true;
        }
    }
    if (selectionChanged)
    {
        if (adjustCells())
            flagAsModified();

        emit cellSelectionChanged(mSelection);
    }

    return true;
}

CSVRender::WorldspaceWidget::dropRequirments CSVRender::PagedWorldspaceWidget::getDropRequirements (CSVRender::WorldspaceWidget::DropType type) const
{
    dropRequirments requirements = WorldspaceWidget::getDropRequirements (type);

    if (requirements!=ignored)
        return requirements;

    switch (type)
    {
        case Type_CellsExterior:
            return canHandle;

        case Type_CellsInterior:
            return needUnpaged;

        default:
            return ignored;
    }
}


unsigned int CSVRender::PagedWorldspaceWidget::getElementMask() const
{
    return WorldspaceWidget::getElementMask() | mControlElements->getSelection();
}

CSVWidget::SceneToolToggle *CSVRender::PagedWorldspaceWidget::makeControlVisibilitySelector (
    CSVWidget::SceneToolbar *parent)
{
    mControlElements = new CSVWidget::SceneToolToggle (parent,
        "Controls & Guides Visibility", ":door.png");

    mControlElements->addButton (":activator.png", Element_CellMarker, ":activator.png",
        "Cell marker");
    mControlElements->addButton (":armor.png", Element_CellArrow, ":armor.png", "Cell arrows");
    mControlElements->addButton (":armor.png", Element_CellBorder, ":armor.png", "Cell border");

    mControlElements->setSelection (0xffffffff);

    connect (mControlElements, SIGNAL (selectionChanged()),
        this, SLOT (elementSelectionChanged()));

    return mControlElements;
}

void CSVRender::PagedWorldspaceWidget::cellDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    /// \todo check if no selected cell is affected and do not update, if that is the case
    if (adjustCells())
        flagAsModified();
}

void CSVRender::PagedWorldspaceWidget::cellRemoved (const QModelIndex& parent, int start,
    int end)
{
    if (adjustCells())
        flagAsModified();
}

void CSVRender::PagedWorldspaceWidget::cellAdded (const QModelIndex& index, int start,
    int end)
{
    /// \todo check if no selected cell is affected and do not update, if that is the case
    if (adjustCells())
        flagAsModified();
}

std::pair<std::string, Ogre::Vector3> CSVRender::PagedWorldspaceWidget::isObjectUnderCursor(float mouseX, float mouseY)
{
    std::pair<std::string, Ogre::Vector3> result = CSVWorld::PhysicsSystem::instance()->castRay(
                                                mouseX, mouseY, NULL, NULL, getCamera());
    if(result.first != "")
    {
        QString name  = QString(result.first.c_str());
        if(!name.contains(QRegExp("^HeightField")))
        {
            std::string sceneNode =
                CSVWorld::PhysicsSystem::instance()->referenceToSceneNode(result.first);

            uint32_t visibilityMask = getCamera()->getViewport()->getVisibilityMask();
            bool ignoreObjects = !(visibilityMask & (uint32_t)CSVRender::Element_Reference);

            if(!ignoreObjects && getSceneManager()->hasSceneNode(sceneNode))
            {
                return result;
            }
        }
    }
    else
        std::cout << "error castRay returned empty " << result.first << std::endl;

    return std::make_pair("", Ogre::Vector3(0,0,0));
}

std::pair<bool, Ogre::Vector3> CSVRender::PagedWorldspaceWidget::mousePositionOnPlane(QMouseEvent *event, Ogre::Plane &plane)
{
    // using a really small value seems to mess up with the projections
    float nearClipDistance = getCamera()->getNearClipDistance(); // save existing
    getCamera()->setNearClipDistance(10.0f);  // arbitrary number
    Ogre::Ray mouseRay = getCamera()->getCameraToViewportRay(
        (float) event->x() / getCamera()->getViewport()->getActualWidth(),
        (float) event->y() / getCamera()->getViewport()->getActualHeight());
    getCamera()->setNearClipDistance(nearClipDistance); // restore
    std::pair<bool, float> planeResult = mouseRay.intersects(plane);

    if(planeResult.first)
        return std::make_pair(true, mouseRay.getPoint(planeResult.second));
    else
        return std::make_pair(false, Ogre::Vector3()); // should only happen if the plane is too small
}

void CSVRender::PagedWorldspaceWidget::debugMousePicking(float mouseX, float mouseY)
{
    CSMSettings::UserSettings &userSettings = CSMSettings::UserSettings::instance();
    bool debug = userSettings.setting ("debug/mouse-picking", QString("false")) == "true" ? true : false;

    std::pair<std::string, Ogre::Vector3> result = CSVWorld::PhysicsSystem::instance()->castRay(
                                                mouseX, mouseY, NULL, NULL, getCamera());
    if(debug && result.first != "")
    {
        // FIXME: is there  a better way to distinguish terrain from objects?
        QString name  = QString(result.first.c_str());
        if(name.contains(QRegExp("^HeightField")))
        {
            // terrain
            std::cout << "terrain: " << result.first << std::endl;
            std::cout << "  hit pos "+ QString::number(result.second.x).toStdString()
                    + ", " + QString::number(result.second.y).toStdString()
                    + ", " + QString::number(result.second.z).toStdString()
                    << std::endl;
        }
        else
        {
            std::string sceneNode =
                CSVWorld::PhysicsSystem::instance()->referenceToSceneNode(result.first);

            uint32_t visibilityMask = getCamera()->getViewport()->getVisibilityMask();
            bool ignoreObjects = !(visibilityMask & (uint32_t)CSVRender::Element_Reference);

            if(!ignoreObjects && getSceneManager()->hasSceneNode(sceneNode))
            {
                if(userSettings.setting("debug/mouse-picking", QString("false")) == "true" ? true : false)
                    updateSelectionHighlight(sceneNode, result.second);
            }

            std::cout << "ReferenceId: " << result.first << std::endl;
            const CSMWorld::CellRef& cellref = mDocument.getData().getReferences().getRecord (result.first).get();
            //std::cout << "CellRef.mId: " << cellref.mId << std::endl; // Same as ReferenceId
            std::cout << "  CellRef.mCell: " << cellref.mCell << std::endl;

            const CSMWorld::RefCollection& references = mDocument.getData().getReferences();
            int index = references.searchId(result.first);
            if (index != -1)
            {
                int columnIndex =
                    references.findColumnIndex(CSMWorld::Columns::ColumnId_ReferenceableId);

                std::cout << "  index: " + QString::number(index).toStdString()
                        +", column index: " + QString::number(columnIndex).toStdString() << std::endl;
            }

            std::map<CSMWorld::CellCoordinates, Cell *>::iterator iter (mCells.begin());
            while (iter!=mCells.end())
            {
                if(iter->first.getId("dummy") == cellref.mCell)
                {
                    //std::cout << "Cell found" << std::endl;
                    break;
                }
                ++iter;
            }
            flagAsModified();
        }
    }
}

// FIXME: for debugging only
void CSVRender::PagedWorldspaceWidget::updateSelectionHighlight(std::string sceneNode, const Ogre::Vector3 &position)
{
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
}
