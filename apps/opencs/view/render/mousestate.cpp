#include "mousestate.hpp"

#include <OgreSceneNode.h>
#include <OgreSceneManager.h>
#include <OgreEntity.h>

#include <OgreMeshManager.h>
#include <OgreManualObject.h>        // FIXME: for debugging
#include <OgreMaterialManager.h>     // FIXME: for debugging
#include <OgreHardwarePixelBuffer.h> // FIXME: for debugging

#include <QMouseEvent>
#include <QElapsedTimer>

#include "../../model/settings/usersettings.hpp"
#include "../world/physicssystem.hpp"

#include "elements.hpp" // FIXME: for debugging
#include "worldspacewidget.hpp"

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

    //plane Z, upvector Y, mOffset z : x-y plane, wheel up/down
    //plane Y, upvector X, mOffset y : y-z plane, wheel left/right
    //plane X, upvector Y, mOffset x : x-z plane, wheel closer/further
    std::pair<Ogre::Vector3, Ogre::Vector3> planeAxis()
    {
        CSMSettings::UserSettings &userSettings = CSMSettings::UserSettings::instance();
        QString wheelDir =  userSettings.setting("debug/mouse-wheel", QString("Closer/Further"));
        if(wheelDir == "Up/Down")
            return std::make_pair(Ogre::Vector3::UNIT_Z, Ogre::Vector3::UNIT_Y);
        else if(wheelDir == "Left/Right")
            return std::make_pair(Ogre::Vector3::UNIT_Y, Ogre::Vector3::UNIT_X);
        else
            return std::make_pair(Ogre::Vector3::UNIT_X, Ogre::Vector3::UNIT_Y);
    }
}

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
        : mParent(parent), mPhysics(parent->getPhysics()), mSceneManager(parent->getSceneManager())
        , mCurrentObj(""), mMouseState(Mouse_Default), mOldPos(0,0), mMouseEventTimer(0), mPlane(0)
        , mGrabbedSceneNode(""), mOrigObjPos(Ogre::Vector3()), mOrigMousePos(Ogre::Vector3())
        , mCurrentMousePos(Ogre::Vector3()), mOffset(0.0f)
    {
        initDebug();

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

        // For debugging only
        std::map<std::string, std::vector<std::string> >::iterator iter = mSelectedEntities.begin();
        for(;iter != mSelectedEntities.end(); ++iter)
        {
            removeHitPoint(mSceneManager, iter->first);

            if(mSceneManager->hasSceneNode(iter->first))
            {
                Ogre::SceneNode *scene = mSceneManager->getSceneNode(iter->first);

                if(scene)
                {
                    scene->removeAndDestroyAllChildren();
                    mSceneManager->destroySceneNode(iter->first);
                }
            }
        }

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
                            //pos.z += mOffset;
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
                    mOffset = 0.0f;
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
                    //mPlane->redefine(Ogre::Vector3::UNIT_Z, result.second);
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

                        // print some debug info
                        if(isDebug())
                        {
                            std::string referenceId = mPhysics->sceneNodeToRefId(result.first);
                            std::cout << "ReferenceId: " << referenceId << std::endl;
                            const CSMWorld::RefCollection& references = mParent->mDocument.getData().getReferences();
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
                        std::pair<Ogre::Vector3, Ogre::Vector3> planeRes = planeAxis();
                        //mOrigObjPos.z += mOffset;
                        Ogre::Vector3 pos = mOrigObjPos+planeRes.first*mOffset+planeResult.second-mOrigMousePos;
                        placeObject(mGrabbedSceneNode, pos);
                        //mCurrentObj = mGrabbedSceneNode; // FIXME
                        mCurrentObj = "";                   // whether the object is selected

                        // reset states
                        mCurrentMousePos = Ogre::Vector3(); // mouse pos to use in wheel event
                        mOrigMousePos = Ogre::Vector3();    // starting pos of mouse in world space
                        mOrigObjPos = Ogre::Vector3();      // starting pos of object in world space
                        mGrabbedSceneNode = "";             // id of the object
                        mOffset = 0.0f;                    // used for z-axis movement
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

    void MouseState::mouseDoubleClickEvent (QMouseEvent *event)
    {
        if(0 && isDebug()) // disable
        {
            // FIXME: OEngine::PhysicEngine creates only one child scene node for the
            // debug drawer.  Hence only the first subview that creates the debug drawer
            // can view the debug lines.  Will need to keep a map in OEngine if multiple
            // subviews are to be supported.
            mPhysics->addSceneManager(mSceneManager, mParent);
            mPhysics->toggleDebugRendering(mSceneManager);
            mParent->flagAsModified();
        }
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
                    //pos.z += mOffset;
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

    // FIXME: for debugging only
    void MouseState::updateSelectionHighlight(const std::string sceneNode, const Ogre::Vector3 &position)
    {
        uint32_t visibilityMask = getViewport()->getVisibilityMask();
        bool ignoreObjects = !(visibilityMask & (uint32_t)CSVRender::Element_Reference);

        if(ignoreObjects || !mSceneManager->hasSceneNode(sceneNode) || !isDebug())
            return;

        CSMSettings::UserSettings &userSettings = CSMSettings::UserSettings::instance();
        bool debugCursor = userSettings.setting(
                    "debug/mouse-position", QString("false")) == "true" ? true : false;

        //TODO: Try http://www.ogre3d.org/tikiwiki/Create+outline+around+a+character
        Ogre::SceneNode *scene = mSceneManager->getSceneNode(sceneNode);
        std::map<std::string, std::vector<std::string> >::iterator iter =
                                                mSelectedEntities.find(sceneNode);
        if(iter != mSelectedEntities.end()) // currently selected
        {
            std::vector<std::string> clonedEntities = mSelectedEntities[sceneNode];
            while(!clonedEntities.empty())
            {
                if(mSceneManager->hasEntity(clonedEntities.back()))
                {
                    scene->detachObject(clonedEntities.back());
                    mSceneManager->destroyEntity(clonedEntities.back());
                }
                clonedEntities.pop_back();
            }
            mSelectedEntities.erase(iter);

            if(debugCursor)
                removeHitPoint(mSceneManager, sceneNode);
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
                if(mSceneManager->hasEntity(entity->getName()+"cover"))
                {
                    // FIXME: this shouldn't really happen... but does :(
                    scene->detachObject(entity->getName()+"cover");
                    mSceneManager->destroyEntity(entity->getName()+"cover");
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
                showHitPoint(mSceneManager, sceneNode, position);
        }
        mParent->flagAsModified();
    }

    void MouseState::placeObject(const std::string sceneNode, const Ogre::Vector3 &pos)
    {
        mSceneManager->getSceneNode(sceneNode)->setPosition(pos);

        // update physics
        std::string refId = mPhysics->sceneNodeToRefId(sceneNode);
        const CSMWorld::CellRef& cellref = mParent->mDocument.getData().getReferences().getRecord (refId).get();
        Ogre::Quaternion xr (Ogre::Radian (-cellref.mPos.rot[0]), Ogre::Vector3::UNIT_X);
        Ogre::Quaternion yr (Ogre::Radian (-cellref.mPos.rot[1]), Ogre::Vector3::UNIT_Y);
        Ogre::Quaternion zr (Ogre::Radian (-cellref.mPos.rot[2]), Ogre::Vector3::UNIT_Z);

        // FIXME: adjustRigidBody() seems to lose objects, work around by deleting and recreating objects
        //mPhysics->moveObject(sceneNode, pos, xr*yr*zr);
        mPhysics->replaceObject(sceneNode, refId, cellref.mScale, pos, xr*yr*zr);

        // update all SceneWidgets and their SceneManagers
        updateSceneWidgets();
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

    bool MouseState::isDebug()
    {
        CSMSettings::UserSettings &userSettings = CSMSettings::UserSettings::instance();

        return userSettings.setting("debug/mouse-picking", QString("false")) == "true" ? true : false;
    }
}
