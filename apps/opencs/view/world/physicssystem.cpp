#include "physicssystem.hpp"

#include <iostream>

#include <OgreRay.h>
#include <OgreCamera.h>
#include <OgreSceneManager.h>

#include <openengine/bullet/physic.hpp>
#include <components/nifbullet/bulletnifloader.hpp>
#include "../../model/settings/usersettings.hpp"
#include "../render/elements.hpp"

namespace CSVWorld
{
    PhysicsSystem::PhysicsSystem()
    {
        // Create physics. shapeLoader is deleted by the physic engine
        NifBullet::ManualBulletShapeLoader* shapeLoader = new NifBullet::ManualBulletShapeLoader();
        mEngine = new OEngine::Physic::PhysicEngine(shapeLoader);
    }

    PhysicsSystem::~PhysicsSystem()
    {
        // FIXME: OEngine does not behave well when multiple instances are created
        // and deleted, sometimes resulting in crashes.  Skip the deletion until the physics
        // code is moved out of OEngine.
        //delete mEngine;
    }

    // looks up the scene manager based on the scene node name (inefficient)
    // NOTE: referenceId is assumed to be unique per document
    // NOTE: searching is done here rather than after rayTest, hence slower to load but
    //       faster to find (guessing, not verified w/ perf test)
    void PhysicsSystem::addObject(const std::string &mesh,
            const std::string &sceneNodeName, const std::string &referenceId, float scale,
            const Ogre::Vector3 &position, const Ogre::Quaternion &rotation, bool placeable)
    {
        bool foundSceneManager = false;
        std::list<Ogre::SceneManager *>::const_iterator iter = mSceneManagers.begin();
        for(; iter != mSceneManagers.end(); ++iter)
        {
            if((*iter)->hasSceneNode(sceneNodeName))
            {
                mSceneNodeToRefId[sceneNodeName] = referenceId;
                mRefIdToSceneNode[referenceId][*iter] = sceneNodeName;
                mSceneNodeToMesh[sceneNodeName] = mesh;
                foundSceneManager = true;
                break;
            }
        }

        if(!foundSceneManager)
        {
            std::cerr << "Attempt to add an object without a corresponding SceneManager: "
                + referenceId + " : " + sceneNodeName << std::endl;

            return;
        }

        // update physics, only one physics model per referenceId
        if(mEngine->getRigidBody(referenceId, true) == NULL)
        {
            mEngine->createAndAdjustRigidBody(mesh,
                    referenceId, scale, position, rotation,
                    0,    // scaledBoxTranslation
                    0,    // boxRotation
                    true, // raycasting
                    placeable);

            // update other scene managers if they have the referenceId (may have moved)
            // NOTE: this part is not needed if object has not moved
            iter = mSceneManagers.begin();
            for(; iter != mSceneManagers.end(); ++iter)
            {
                std::string name = refIdToSceneNode(referenceId, *iter);
                if(name != sceneNodeName && (*iter)->hasSceneNode(name))
                {
                        // FIXME: rotation or scale not updated
                    (*iter)->getSceneNode(name)->setPosition(position);
                }
            }
        }
    }

    // normal delete (e.g closing a scene subview)
    // TODO: should think about using some kind of reference counting within RigidBody
    void PhysicsSystem::removeObject(const std::string &sceneNodeName, bool force)
    {
        std::string referenceId = mSceneNodeToRefId[sceneNodeName];

        if(referenceId != "")
        {
            mSceneNodeToRefId.erase(sceneNodeName);
            mSceneNodeToMesh.erase(sceneNodeName);

            // find which SceneManager has this object
            Ogre::SceneManager *sceneManager = NULL;
            std::list<Ogre::SceneManager *>::const_iterator iter = mSceneManagers.begin();
            for(; iter != mSceneManagers.end(); ++iter)
            {
                if((*iter)->hasSceneNode(sceneNodeName))
                {
                    sceneManager = *iter;
                    break;
                }
            }

            if(!sceneManager)
            {
                std::cerr << "Attempt to remove an object without a corresponding SceneManager: "
                    + sceneNodeName << std::endl;

                return;
            }

            // illustration: erase the object "K" from the object map
            //
            // RidigBody          SubView          Ogre
            // ---------------    --------------   -------------
            // ReferenceId "A"   (SceneManager X   SceneNode "J")
            //                   (SceneManager Y   SceneNode "K")  <--- erase
            //                   (SceneManager Z   SceneNode "L")
            //
            // ReferenceId "B"   (SceneManager X   SceneNode "M")
            //                   (SceneManager Y   SceneNode "N")  <--- notice not deleted
            //                   (SceneManager Z   SceneNode "O")
            std::map<std::string, std::map<Ogre::SceneManager *, std::string> >::iterator itRef =
                mRefIdToSceneNode.begin();
            for(; itRef != mRefIdToSceneNode.end(); ++itRef)
            {
                if((*itRef).second.find(sceneManager) != (*itRef).second.end())
                {
                    (*itRef).second.erase(sceneManager);
                    break;
                }
            }

            // check whether the physics model be deleted
            if(force || mRefIdToSceneNode.find(referenceId) == mRefIdToSceneNode.end())
            {
                mEngine->removeRigidBody(referenceId);
                mEngine->deleteRigidBody(referenceId);
            }
        }
    }

    void PhysicsSystem::replaceObject(const std::string &sceneNodeName,
            const std::string &referenceId, float scale, const Ogre::Vector3 &position,
            const Ogre::Quaternion &rotation, bool placeable)
    {
        std::string mesh = mSceneNodeToMesh[sceneNodeName];
        removeObject(sceneNodeName, true); // force delete
        addObject(mesh, sceneNodeName, referenceId, scale, position, rotation, placeable);
    }

    void PhysicsSystem::moveObject(const std::string &sceneNodeName,
            const Ogre::Vector3 &position, const Ogre::Quaternion &rotation)
    {
        mEngine->adjustRigidBody(mEngine->getRigidBody(sceneNodeName, true /*raycasting*/),
                position, rotation);
    }

    void PhysicsSystem::moveSceneNodeImpl(const std::string sceneNodeName,
            const std::string referenceId, const Ogre::Vector3 &position)
    {
        std::list<Ogre::SceneManager *>::const_iterator iter = mSceneManagers.begin();
        for(; iter != mSceneManagers.end(); ++iter)
        {
            std::string name = refIdToSceneNode(referenceId, *iter);
            if(name != sceneNodeName && (*iter)->hasSceneNode(name))
            {
                (*iter)->getSceneNode(name)->setPosition(position);
            }
        }
    }

    void PhysicsSystem::moveSceneNodes(const std::string sceneNodeName, const Ogre::Vector3 &position)
    {
        moveSceneNodeImpl(sceneNodeName, sceneNodeToRefId(sceneNodeName), position);
    }
    void PhysicsSystem::addHeightField(Ogre::SceneManager *sceneManager,
            float* heights, int x, int y, float yoffset, float triSize, float sqrtVerts)
    {
        std::string name = "HeightField_"
            + QString::number(x).toStdString() + "_" + QString::number(y).toStdString();

        if(mTerrain.find(name) == mTerrain.end())
            mEngine->addHeightField(heights, x, y, yoffset, triSize, sqrtVerts);

        mTerrain.insert(std::pair<std::string, Ogre::SceneManager *>(name, sceneManager));
    }

    void PhysicsSystem::removeHeightField(Ogre::SceneManager *sceneManager, int x, int y)
    {
        std::string name = "HeightField_"
            + QString::number(x).toStdString() + "_" + QString::number(y).toStdString();

        if(mTerrain.count(name) == 1)
            mEngine->removeHeightField(x, y);

        std::multimap<std::string, Ogre::SceneManager *>::iterator iter = mTerrain.begin();
        for(; iter != mTerrain.end(); ++iter)
        {
            if((*iter).second == sceneManager)
            {
                mTerrain.erase(iter);
                break;
            }
        }
    }

    // sceneMgr: to lookup the scene node name from the object's referenceId
    // camera: primarily used to get the visibility mask for the viewport
    //
    // returns the found object's scene node name and its position in the world space
    //
    // WARNING: far clip distance is a global setting, if it changes in future
    //          this method will need to be updated
    std::pair<std::string, Ogre::Vector3> PhysicsSystem::castRay(float mouseX,
            float mouseY, Ogre::SceneManager *sceneMgr, Ogre::Camera *camera)
    {
        // NOTE: there could be more than one camera for the scene manager
        // TODO: check whether camera belongs to sceneMgr
        if(!sceneMgr || !camera || !camera->getViewport())
            return std::make_pair("", Ogre::Vector3(0,0,0)); // FIXME: this should be an exception

        // using a really small value seems to mess up with the projections
        float nearClipDistance = camera->getNearClipDistance(); // save existing
        camera->setNearClipDistance(10.0f);  // arbitrary number
        Ogre::Ray ray = camera->getCameraToViewportRay(mouseX, mouseY);
        camera->setNearClipDistance(nearClipDistance); // restore

        Ogre::Vector3 from = ray.getOrigin();
        CSMSettings::UserSettings &userSettings = CSMSettings::UserSettings::instance();
        float farClipDist = userSettings.setting("Scene/far clip distance", QString("300000")).toFloat();
        Ogre::Vector3 to = ray.getPoint(farClipDist);

        btVector3 _from, _to;
        _from = btVector3(from.x, from.y, from.z);
        _to = btVector3(to.x, to.y, to.z);

        uint32_t visibilityMask = camera->getViewport()->getVisibilityMask();
        bool ignoreHeightMap = !(visibilityMask & (uint32_t)CSVRender::Element_Terrain);
        bool ignoreObjects = !(visibilityMask & (uint32_t)CSVRender::Element_Reference);

        Ogre::Vector3 norm; // not used
        std::pair<std::string, float> result =
                                mEngine->rayTest(_from, _to, !ignoreObjects, ignoreHeightMap, &norm);

        // result.first is the object's referenceId
        if(result.first == "")
            return std::make_pair("", Ogre::Vector3(0,0,0));
        else
        {
            std::string name = refIdToSceneNode(result.first, sceneMgr);
            if(name == "")
                name = result.first;
            else
                name = refIdToSceneNode(result.first, sceneMgr);

            return std::make_pair(name, ray.getPoint(farClipDist*result.second));
        }
    }

    std::string PhysicsSystem::refIdToSceneNode(std::string referenceId, Ogre::SceneManager *sceneMgr)
    {
        return mRefIdToSceneNode[referenceId][sceneMgr];
    }

    std::string PhysicsSystem::sceneNodeToRefId(std::string sceneNodeName)
    {
        return mSceneNodeToRefId[sceneNodeName];
    }

    void PhysicsSystem::addSceneManager(Ogre::SceneManager *sceneMgr, CSVRender::SceneWidget * sceneWidget)
    {
        mSceneManagers.push_back(sceneMgr);
        mSceneWidgets[sceneMgr] = sceneWidget;
    }

    std::map<Ogre::SceneManager*, CSVRender::SceneWidget *> PhysicsSystem::sceneWidgets()
    {
        return mSceneWidgets;
    }

    void PhysicsSystem::removeSceneManager(Ogre::SceneManager *sceneMgr)
    {
        std::list<Ogre::SceneManager *>::iterator iter = mSceneManagers.begin();
        for(; iter != mSceneManagers.end(); ++iter)
        {
            if(*iter == sceneMgr)
            {
                mSceneManagers.erase(iter);
                break;
            }
        }
        mSceneWidgets.erase(sceneMgr);
    }

    void PhysicsSystem::toggleDebugRendering(Ogre::SceneManager *sceneMgr)
    {
        // FIXME: should check if sceneMgr is in the list
        if(!sceneMgr)
            return;

        mEngine->setSceneManager(sceneMgr);

        CSMSettings::UserSettings &userSettings = CSMSettings::UserSettings::instance();
        if(!(userSettings.setting("debug/mouse-picking", QString("false")) == "true" ? true : false))
        {
            std::cerr << "Turn on mouse-picking debug option to see collision shapes." << std::endl;
            return;
        }

        mEngine->toggleDebugRendering();
        mEngine->stepSimulation(0.0167); // DebugDrawer::step() not directly accessible
    }
}
