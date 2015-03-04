#include "mousestate.hpp"

#include <OgreSceneNode.h>
#include <OgreSceneManager.h>
#include <OgreEntity.h>
#include <OgreMeshManager.h>

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
        : mParent(parent), mPhysics(parent->mDocument.getPhysics()), mSceneManager(parent->getSceneManager())
        , mCurrentObj(""), mMouseState(Mouse_Default), mOldPos(0,0), mMouseEventTimer(0), mPlane(0)
        , mGrabbedSceneNode(""), mOrigObjPos(Ogre::Vector3()), mOrigMousePos(Ogre::Vector3())
        , mCurrentMousePos(Ogre::Vector3()), mOffset(0.0f)
        , mColIndexPosX(0), mColIndexPosY(0), mColIndexPosZ(0), mIdTableModel(0)
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
            300000,300000, // FIXME: use far clip dist?
            1,1, // segments
            true,  // normals
            1,     // numTexCoordSets
            1,1, // uTile, vTile
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
                            std::pair<Ogre::Vector3, Ogre::Vector3> planeRes = planeAxis();
                            Ogre::Vector3 pos = mOrigObjPos + planeRes.first*mOffset;
                            mSceneManager->getSceneNode(mGrabbedSceneNode)->setPosition(pos+planeResult.second-mOrigMousePos);
                            mCurrentMousePos = planeResult.second;
                            mPhysics->moveSceneNodes(mGrabbedSceneNode, pos+planeResult.second-mOrigMousePos);
                            updateSceneWidgets();
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
                    std::pair<std::string, Ogre::Vector3> result = objectUnderCursor(event->x(), event->y());
                    if(result.first == "")
                        break;

                    mGrabbedSceneNode = result.first;
                    // ray test agaist the plane to get a starting position of the
                    // mouse in relation to the object position
                    std::pair<Ogre::Vector3, Ogre::Vector3> planeRes = planeAxis();
                    mPlane->redefine(planeRes.first, result.second);
                    std::pair<bool, Ogre::Vector3> planeResult = mousePositionOnPlane(event->pos(), *mPlane);
                    if(planeResult.first)
                    {
                        mOrigMousePos = planeResult.second;
                        mCurrentMousePos = planeResult.second;
                        mOffset = 0.0f;
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

                    }
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
                        std::pair<Ogre::Vector3, Ogre::Vector3> planeRes = planeAxis();
                        Ogre::Vector3 pos = mOrigObjPos+planeRes.first*mOffset+planeResult.second-mOrigMousePos;
                        // use the saved scene node name since the physics model has not moved yet
                        std::string referenceId = mPhysics->sceneNodeToRefId(mGrabbedSceneNode);

                        mParent->mDocument.getUndoStack().beginMacro (QObject::tr("Move Object"));
                        mParent->mDocument.getUndoStack().push(new CSMWorld::ModifyCommand(*mIdTableModel,
                            mIdTableModel->getModelIndex(referenceId, mColIndexPosX), pos.x));
                        mParent->mDocument.getUndoStack().push(new CSMWorld::ModifyCommand(*mIdTableModel,
                            mIdTableModel->getModelIndex(referenceId, mColIndexPosY), pos.y));
                        mParent->mDocument.getUndoStack().push(new CSMWorld::ModifyCommand(*mIdTableModel,
                            mIdTableModel->getModelIndex(referenceId, mColIndexPosZ), pos.z));
                        mParent->mDocument.getUndoStack().endMacro();

                        // FIXME: highlight current object?
                        //mCurrentObj = mGrabbedSceneNode; // FIXME: doesn't work?
                        mCurrentObj = "";                   // whether the object is selected

                        mMouseState = Mouse_Edit;

                        // reset states
                        mCurrentMousePos = Ogre::Vector3(); // mouse pos to use in wheel event
                        mOrigMousePos = Ogre::Vector3();    // starting pos of mouse in world space
                        mOrigObjPos = Ogre::Vector3();      // starting pos of object in world space
                        mGrabbedSceneNode = "";             // id of the object
                        mOffset = 0.0f;                     // used for z-axis movement
                        mOldPos = QPoint(0, 0);             // to calculate relative movement of mouse
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
                    // FIXME: terrain editing goes here
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
                // move the object along the z axis during Mouse_Drag or Mouse_Grab
                if (event->delta())
                {
                    // seems positive is up and negative is down
                    mOffset += (event->delta()/1); // FIXME: arbitrary number, make config option?

                    std::pair<Ogre::Vector3, Ogre::Vector3> planeRes = planeAxis();
                    Ogre::Vector3 pos = mOrigObjPos + planeRes.first*mOffset;
                    mSceneManager->getSceneNode(mGrabbedSceneNode)->setPosition(pos+mCurrentMousePos-mOrigMousePos);
                    mPhysics->moveSceneNodes(mGrabbedSceneNode, pos+mCurrentMousePos-mOrigMousePos);
                    updateSceneWidgets();
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
                mCurrentMousePos = Ogre::Vector3();
                mOrigMousePos = Ogre::Vector3();
                mOrigObjPos = Ogre::Vector3();
                mGrabbedSceneNode = "";
                mCurrentObj = "";
                mOldPos = QPoint(0, 0);
                mMouseEventTimer->invalidate();
                mOffset = 0.0f;

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

    std::pair<bool, Ogre::Vector3> MouseState::mousePositionOnPlane(const QPoint &pos, const Ogre::Plane &plane)
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

    std::pair<std::string, Ogre::Vector3> MouseState::terrainUnderCursor(const int mouseX, const int mouseY)
    {
        if(!getViewport())
            return std::make_pair("", Ogre::Vector3());

        float x = (float) mouseX / getViewport()->getActualWidth();
        float y = (float) mouseY / getViewport()->getActualHeight();

        std::pair<std::string, Ogre::Vector3> result = mPhysics->castRay(x, y, mSceneManager, getCamera());
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

    std::pair<std::string, Ogre::Vector3> MouseState::objectUnderCursor(const int mouseX, const int mouseY)
    {
        if(!getViewport())
            return std::make_pair("", Ogre::Vector3());

        float x = (float) mouseX / getViewport()->getActualWidth();
        float y = (float) mouseY / getViewport()->getActualHeight();

        std::pair<std::string, Ogre::Vector3> result = mPhysics->castRay(x, y, mSceneManager, getCamera());
        if(result.first != "")
        {
            // NOTE: anything not terrain is assumed to be an object
            QString name  = QString(result.first.c_str());
            if(!name.contains(QRegExp("^HeightField")))
            {
                uint32_t visibilityMask = getViewport()->getVisibilityMask();
                bool ignoreObjects = !(visibilityMask & (uint32_t)CSVRender::Element_Reference);

                if(!ignoreObjects && mSceneManager->hasSceneNode(result.first))
                {
                    return result;
                }
            }
        }

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
}
