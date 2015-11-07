#include "mousestate.hpp"

#include <OgreSceneNode.h>
#include <OgreSceneManager.h>
#include <OgreEntity.h>
#include <OgreMeshManager.h>
#include <OgreCamera.h>
#include <OgreViewport.h>

#include <QMouseEvent>
#include <QElapsedTimer>
#include <QObject>

#include "../../model/settings/usersettings.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/universalid.hpp"
#include "../world/physicssystem.hpp"

#include "elements.hpp"
#include "worldspacewidget.hpp"

namespace CSVRender
{
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

    MouseState::MouseState(WorldspaceWidget *parent)
        : mMouseState(Mouse_Default), mParent(parent), mPhysics(parent->mDocument.getPhysics())
        , mSceneManager(parent->getSceneManager()), mOldCursorPos(0,0), mCurrentObj(""), mGrabbedSceneNode(""), mGrabbedRefId("")
        , mMouseEventTimer(0), mPlane(0), mOrigObjPos(Ogre::Vector3()), mOrigMousePos(Ogre::Vector3())
        , mOldMousePos(Ogre::Vector3()), mIdTableModel(0), mColIndexPosX(0)
        , mColIndexPosY(0), mColIndexPosZ(0)
    {
        const CSMWorld::RefCollection& references = mParent->mDocument.getData().getReferences();

        mColIndexPosX = references.findColumnIndex(CSMWorld::Columns::ColumnId_PositionXPos);
        mColIndexPosY = references.findColumnIndex(CSMWorld::Columns::ColumnId_PositionYPos);
        mColIndexPosZ = references.findColumnIndex(CSMWorld::Columns::ColumnId_PositionZPos);

        mIdTableModel = static_cast<CSMWorld::IdTable *>(
            mParent->mDocument.getData().getTableModel(CSMWorld::UniversalId::Type_Reference));

        mMouseEventTimer = new QElapsedTimer();
        mMouseEventTimer->invalidate();

        std::pair<Ogre::Vector3, Ogre::Vector3> planeRes = planeAxis();
        mPlane = new Ogre::Plane(planeRes.first, 0);
        Ogre::MeshPtr mesh = Ogre::MeshManager::getSingleton().createPlane("mouse",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            *mPlane,
            300000,300000,  // FIXME: use far clip dist?
            1,1,            // segments
            true,           // normals
            1,              // numTexCoordSets
            1,1,            // uTile, vTile
            planeRes.second // upVector
            );
    }

    MouseState::~MouseState ()
    {
        delete mMouseEventTimer;
        delete mPlane;
    }

    void MouseState::mouseMoveEvent (QMouseEvent *event)
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
                if(event->pos() != mOldCursorPos) // TODO: maybe don't update less than a quantum?
                {
                    mOldCursorPos = event->pos();

                    // ray test against the plane to provide feedback to the user the
                    // relative movement of the object on the movement plane
                    std::pair<bool, Ogre::Vector3> planeResult = mousePosOnPlane(event->pos(), *mPlane);
                    if(planeResult.first)
                    {
                        Ogre::Vector3 pos = mOrigObjPos + (planeResult.second-mOrigMousePos);

                        mSceneManager->getSceneNode(mGrabbedSceneNode)->setPosition(pos);
                        mPhysics->moveSceneNodes(mGrabbedSceneNode, pos);
                        updateSceneWidgets();

                        mOldMousePos = planeResult.second;
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

    void MouseState::mousePressEvent (QMouseEvent *event)
    {
        switch(mMouseState)
        {
            case Mouse_Grab:
            case Mouse_Drag:
            {
                break;
            }
            case Mouse_Edit:
            case Mouse_Default:
            {
                if(event->buttons() & Qt::RightButton)
                {
                    // get object or pathgrid
                    std::pair<std::string, Ogre::Vector3> result = underCursor(event->x(), event->y(),
                                CSVRender::Element_Reference|CSVRender::Element_Pathgrid);

                    if(result.first == "")
                        break;

                    mGrabbedSceneNode = mPhysics->refIdToSceneNode(result.first, mSceneManager);
                    if(!mSceneManager->hasSceneNode(mGrabbedSceneNode))
                        break;

                    mGrabbedRefId = result.first;
                    // ray test agaist the plane to get a starting position of the
                    // mouse in relation to the object position
                    std::pair<Ogre::Vector3, Ogre::Vector3> planeRes = planeAxis();
                    mPlane->redefine(planeRes.first, result.second);
                    std::pair<bool, Ogre::Vector3> planeResult = mousePosOnPlane(event->pos(), *mPlane);
                    if(planeResult.first)
                    {
                        mOrigMousePos = planeResult.second;
                        mOldMousePos = planeResult.second;
                    }

                    mOrigObjPos = mSceneManager->getSceneNode(mGrabbedSceneNode)->getPosition();
                    mMouseEventTimer->start();

                    mMouseState = Mouse_Grab;
                }
                break;
            }
            /* NO_DEFAULT_CASE */
        }
    }

    void MouseState::mouseReleaseEvent (QMouseEvent *event)
    {
        switch(mMouseState)
        {
            case Mouse_Grab:
            {
                std::pair<std::string, Ogre::Vector3> result = underCursor(event->x(), event->y(),
                            CSVRender::Element_Reference|CSVRender::Element_Pathgrid);

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

                    }
//#if 0
                    // print some debug info
                    std::cout << "result grab release: " << result.first << std::endl;
                    std::cout << "  hit pos "+ QString::number(result.second.x).toStdString()
                            + ", " + QString::number(result.second.y).toStdString()
                            + ", " + QString::number(result.second.z).toStdString() << std::endl;
//#endif
                }
                break;
            }
            case Mouse_Drag:
            {
                // final placement
                std::pair<bool, Ogre::Vector3> planeResult = mousePosOnPlane(event->pos(), *mPlane);
                if(planeResult.first)
                {
                    Ogre::Vector3 pos = mOrigObjPos + (planeResult.second-mOrigMousePos);

                    // use the saved reference Id since the physics model has not moved yet
                    if(QString(mGrabbedRefId.c_str()).contains(QRegExp("^Pathgrid")))
                    {
                        // FIXME: move pathgrid point, but don't save yet (need pathgrid
                        // table feature & its data structure to be completed)
                        // Also need to signal PathgridPoint object of change
                        std::pair<std::string, float> result =
                            mPhysics->distToClosest(pos,
                                    getCamera()->getViewport()->getVisibilityMask(), 600); // snap

                        if(result.first != "" && // don't allow pathgrid points under the cursor
                            !QString(result.first.c_str()).contains(QRegExp("^Pathgrid")))
                        {
                            pos.z -= result.second;
                            pos.z += 1; // arbitrary number, lift up slightly (maybe change the nif?)
                            // FIXME: rather than just updating at the end, should
                            // consider providing visual feedback of terrain height
                            // while dragging the pathgrid point (maybe check whether
                            // the object is a pathgrid point at the begging and set
                            // a flag?)
                            placeObject(mGrabbedSceneNode, pos); // result.second
                            mParent->pathgridMoved(mGrabbedRefId, pos); // result.second
                        }
                        else
                            cancelDrag(); // FIXME: does not allow editing if terrain not visible
                    }
                    else
                    {
                        mParent->mDocument.getUndoStack().beginMacro (QObject::tr("Move Object"));
                        mParent->mDocument.getUndoStack().push(new CSMWorld::ModifyCommand(*mIdTableModel,
                            mIdTableModel->getModelIndex(mGrabbedRefId, mColIndexPosX), pos.x));
                        mParent->mDocument.getUndoStack().push(new CSMWorld::ModifyCommand(*mIdTableModel,
                            mIdTableModel->getModelIndex(mGrabbedRefId, mColIndexPosY), pos.y));
                        mParent->mDocument.getUndoStack().push(new CSMWorld::ModifyCommand(*mIdTableModel,
                            mIdTableModel->getModelIndex(mGrabbedRefId, mColIndexPosZ), pos.z));
                        mParent->mDocument.getUndoStack().endMacro();
                    }

                    // FIXME: highlight current object?
                    //mCurrentObj = mGrabbedRefId; // FIXME: doesn't work?
                    mCurrentObj = "";                   // whether the object is selected

                    mMouseState = Mouse_Edit;

                    // reset states
                    mGrabbedRefId = "";              // id of the object
                    mGrabbedSceneNode = "";
                    mOrigMousePos = Ogre::Vector3(); // starting pos of mouse in world space
                    mOrigObjPos =   Ogre::Vector3(); // starting pos of object in world space
                    mOldMousePos =  Ogre::Vector3(); // mouse pos to use in wheel event
                    mOldCursorPos = QPoint(0, 0);    // to calculate relative movement of mouse
            }
                break;
            }
            case Mouse_Edit:
            case Mouse_Default:
            {
                // probably terrain, check
                std::pair<std::string, Ogre::Vector3> result = underCursor(event->x(), event->y(),
                        CSVRender::Element_Terrain);

                if(result.first != "")
                {
                    // FIXME: terrain editing goes here
//#if 0
                    std::cout << "result default/edit release: " << result.first << std::endl;
                    std::cout << "  hit pos "+ QString::number(result.second.x).toStdString()
                            + ", " + QString::number(result.second.y).toStdString()
                            + ", " + QString::number(result.second.z).toStdString() << std::endl;
//#endif
                }
                break;
            }
            /* NO_DEFAULT_CASE */
        }
        mMouseEventTimer->invalidate();
    }

    void MouseState::mouseDoubleClickEvent (QMouseEvent *event)
    {
        event->ignore();
        //mPhysics->toggleDebugRendering(mSceneManager);
        //mParent->flagAsModified();
    }

    bool MouseState::wheelEvent (QWheelEvent *event)
    {
        switch(mMouseState)
        {
            case Mouse_Grab:
                mMouseState = Mouse_Drag;

                /* FALL_THROUGH */
            case Mouse_Drag:
            {
                // move the object along the axis normal to the plane during Mouse_Drag or Mouse_Grab
                if (event->delta())
                {
                    // The mouse point is where the mouse points the object when dragging starts.
                    // The object position is usually a little away from the mount point.

                    // Get the new world position of mouse on the plane offset from the wheel
                    // FIXME: make the sensitivity a user setting and/or allow modifiers
                    std::pair<Ogre::Vector3, Ogre::Vector3> planeRes = planeAxis();
                    Ogre::Vector3 mousePos = mOldMousePos + planeRes.first*(event->delta()/1.5);

                    // Move the movement plane to the new mouse position. The plane is created on
                    // the mouse point (i.e. not the object position)
                    mPlane->redefine(planeRes.first, mousePos);

                    // Calculate the new screen position of the cursor
                    Ogre::Vector3 screenPos =
                        getCamera()->getProjectionMatrix() * getCamera()->getViewMatrix() * mousePos;
                    int screenX = (screenPos.x/2+0.5) * getViewport()->getActualWidth();
                    int screenY = (1-(screenPos.y/2+0.5)) * getViewport()->getActualHeight();

                    // Move the cursor to the new screen position
                    QCursor::setPos(mParent->mapToGlobal(QPoint(screenX, screenY)));

                    // Use the new position to check the world position of the mouse
                    std::pair<bool, Ogre::Vector3> planeResult =
                        mousePosOnPlane(QPoint(screenX, screenY), *mPlane);
                    {
                        if(planeResult.first)
                            mousePos = planeResult.second;
                    }

                    // Find the final world position of the object
                    Ogre::Vector3 finalPos = mOrigObjPos + (mousePos-mOrigMousePos);

                    // update Ogre and Bullet
                    mSceneManager->getSceneNode(mGrabbedSceneNode)->setPosition(finalPos);
                    mPhysics->moveSceneNodes(mGrabbedSceneNode, finalPos);
                    updateSceneWidgets();

                    // remember positions for next time
                    mOldMousePos = mousePos;
                    mOldCursorPos = QPoint(screenX, screenY);
                }
                break;
            }
            case Mouse_Edit:
            case Mouse_Default:
            {
                return false;
            }
            /* NO_DEFAULT_CASE */
        }

        return true;
    }

    void MouseState::cancelDrag()
    {
        switch(mMouseState)
        {
            case Mouse_Grab:
            case Mouse_Drag:
            {
                // cancel operation & return the object to the original position
                mSceneManager->getSceneNode(mGrabbedSceneNode)->setPosition(mOrigObjPos);
                // update all SceneWidgets and their SceneManagers
                mPhysics->moveSceneNodes(mGrabbedSceneNode, mOrigObjPos);
                updateSceneWidgets();

                // reset states
                mMouseState = Mouse_Default;
                mOldMousePos = Ogre::Vector3();
                mOrigMousePos = Ogre::Vector3();
                mOrigObjPos = Ogre::Vector3();
                mGrabbedRefId = "";
                mGrabbedSceneNode = "";
                mCurrentObj = "";
                mOldCursorPos = QPoint(0, 0);
                mMouseEventTimer->invalidate();

                break;
            }
            case Mouse_Edit:
            case Mouse_Default:
            {
                break;
            }
            /* NO_DEFAULT_CASE */
        }
    }

    //plane Z, upvector Y, mOffset z : x-y plane, wheel up/down
    //plane Y, upvector X, mOffset y : y-z plane, wheel left/right
    //plane X, upvector Y, mOffset x : x-z plane, wheel closer/further
    std::pair<Ogre::Vector3, Ogre::Vector3> MouseState::planeAxis()
    {
        const bool screenCoord =  true;
        Ogre::Vector3 dir = getCamera()->getDerivedDirection();

        QString wheelDir =  "Closer/Further";
        if(wheelDir == "Left/Right")
        {
            if(screenCoord)
                return std::make_pair(getCamera()->getDerivedRight(), getCamera()->getDerivedUp());
            else
                return std::make_pair(Ogre::Vector3::UNIT_Y, Ogre::Vector3::UNIT_Z);
        }
        else if(wheelDir == "Up/Down")
        {
            if(screenCoord)
                return std::make_pair(getCamera()->getDerivedUp(), Ogre::Vector3(-dir.x, -dir.y, -dir.z));
            else
                return std::make_pair(Ogre::Vector3::UNIT_Z, Ogre::Vector3::UNIT_X);
        }
        else
        {
            if(screenCoord)
                return std::make_pair(Ogre::Vector3(-dir.x, -dir.y, -dir.z), getCamera()->getDerivedRight());
            else
                return std::make_pair(Ogre::Vector3::UNIT_X, Ogre::Vector3::UNIT_Y);
        }
    }

    std::pair<bool, Ogre::Vector3> MouseState::mousePosOnPlane(const QPoint &pos, const Ogre::Plane &plane)
    {
        // using a really small value seems to mess up with the projections
        float nearClipDistance = getCamera()->getNearClipDistance(); // save existing
        getCamera()->setNearClipDistance(10.0f);  // arbitrary number
        Ogre::Ray mouseRay = getCamera()->getCameraToViewportRay(
            (float) pos.x() / getViewport()->getActualWidth(),
            (float) pos.y() / getViewport()->getActualHeight());
        getCamera()->setNearClipDistance(nearClipDistance); // restore
        std::pair<bool, float> planeResult = mouseRay.intersects(plane);

        if(planeResult.first)
            return std::make_pair(true, mouseRay.getPoint(planeResult.second));
        else
            return std::make_pair(false, Ogre::Vector3()); // should only happen if the plane is too small
    }

    std::pair<std::string, Ogre::Vector3> MouseState::underCursor(const int mouseX,
            const int mouseY, Ogre::uint32 elements)
    {
        if(!getViewport())
            return std::make_pair("", Ogre::Vector3());

        float x = (float) mouseX / getViewport()->getActualWidth();
        float y = (float) mouseY / getViewport()->getActualHeight();

        std::pair<std::string, Ogre::Vector3> result = mPhysics->castRay(x, y, mSceneManager, getCamera());

        if(result.first != "" &&
            ((elements & (Ogre::uint32)CSVRender::Element_Terrain &&
            QString(result.first.c_str()).contains(QRegExp("^Height"))) ||
            (elements & (Ogre::uint32)CSVRender::Element_Reference &&
            QString(result.first.c_str()).contains(QRegExp("^ref#"))) ||
            (elements & (Ogre::uint32)CSVRender::Element_Pathgrid &&
            QString(result.first.c_str()).contains(QRegExp("^Pathgrid")))))
        {
            return result;
        }
        else
            return std::make_pair("", Ogre::Vector3());
    }

    void MouseState::updateSceneWidgets()
    {
        std::map<Ogre::SceneManager*, CSVRender::SceneWidget *> sceneWidgets = mPhysics->sceneWidgets();

        std::map<Ogre::SceneManager*, CSVRender::SceneWidget *>::iterator iter = sceneWidgets.begin();
        for(; iter != sceneWidgets.end(); ++iter)
        {
            (*iter).second->updateScene();
        }
    }

    Ogre::Camera *MouseState::getCamera()
    {
        return mParent->getCamera();
    }

    Ogre::Viewport *MouseState::getViewport()
    {
        return mParent->getCamera()->getViewport();
    }

    void MouseState::placeObject(const std::string sceneNodeName, const Ogre::Vector3 &pos)
    {
        mSceneManager->getSceneNode(sceneNodeName)->setPosition(pos);

        // update physics
        mPhysics->replaceObject(sceneNodeName, 1, pos, Ogre::Quaternion::IDENTITY);

        // update all SceneWidgets and their SceneManagers
        updateSceneWidgets();
    }
}
