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
    PhysicsSystem *PhysicsSystem::mPhysicsSystemInstance = 0;

    PhysicsSystem::PhysicsSystem() : mSceneMgr(0)
    {
        assert(!mPhysicsSystemInstance);
        mPhysicsSystemInstance = this;

        // Create physics. shapeLoader is deleted by the physic engine
        NifBullet::ManualBulletShapeLoader* shapeLoader = new NifBullet::ManualBulletShapeLoader();
        mEngine = new OEngine::Physic::PhysicEngine(shapeLoader);
    }

    PhysicsSystem::~PhysicsSystem()
    {
        delete mEngine;
    }

    PhysicsSystem *PhysicsSystem::instance()
    {
        assert(mPhysicsSystemInstance);
        return mPhysicsSystemInstance;
    }

    // FIXME: looks up the scene manager based on the scene node name (highly inefficient)
    // NOTE: referenceId is assumed to be unique per document
    // NOTE: searching is done here rather than after rayTest, so slower to load but
    // faster to find (not verified w/ perf test)
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

        if(foundSceneManager)
        {
            mEngine->createAndAdjustRigidBody(mesh,
                    referenceId, scale, position, rotation,
                    0,    // scaledBoxTranslation
                    0,    // boxRotation
                    true, // raycasting
                    placeable);
        }
    }

    void PhysicsSystem::removeObject(const std::string &sceneNodeName)
    {
        mEngine->removeRigidBody(sceneNodeName);
        mEngine->deleteRigidBody(sceneNodeName);
    }

    void PhysicsSystem::moveObject(const std::string &sceneNodeName,
            const Ogre::Vector3 &position, const Ogre::Quaternion &rotation)
    {
        mEngine->adjustRigidBody(mEngine->getRigidBody(sceneNodeName, true /*raycasting*/),
                position, rotation);
    }

    void PhysicsSystem::addHeightField(float* heights,
            int x, int y, float yoffset, float triSize, float sqrtVerts)
    {
        mEngine->addHeightField(heights, x, y, yoffset, triSize, sqrtVerts);
    }

    void PhysicsSystem::removeHeightField(int x, int y)
    {
        mEngine->removeHeightField(x, y);
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
            return std::make_pair(refIdToSceneNode(result.first, sceneMgr), ray.getPoint(farClipDist*result.second));
    }

    std::string PhysicsSystem::refIdToSceneNode(std::string referenceId, Ogre::SceneManager *sceneMgr)
    {
        return mRefIdToSceneNode[referenceId][sceneMgr];
    }

    std::string PhysicsSystem::sceneNodeToRefId(std::string sceneNodeName)
    {
        return mSceneNodeToRefId[sceneNodeName];
    }

    std::string PhysicsSystem::sceneNodeToMesh(std::string sceneNodeName)
    {
        return mSceneNodeToMesh[sceneNodeName];
    }

    void PhysicsSystem::addSceneManager(Ogre::SceneManager *sceneMgr)
    {
        mSceneManagers.push_back(sceneMgr);
    }

    void PhysicsSystem::toggleDebugRendering(Ogre::SceneManager *sceneMgr)
    {
        // FIXME: should check if sceneMgr is in the list
        if(!mSceneMgr)
            return; // FIXME: maybe this should be an exception

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
