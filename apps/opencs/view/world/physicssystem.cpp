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

    PhysicsSystem::PhysicsSystem(Ogre::SceneManager *sceneMgr) : mSceneMgr(sceneMgr)
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

    void PhysicsSystem::addObject(const std::string &mesh,
            const std::string &name, const std::string &referenceId, float scale,
            const Ogre::Vector3 &position, const Ogre::Quaternion &rotation, bool placeable)
    {
        mRefToSceneNode[referenceId] = name;

        mEngine->createAndAdjustRigidBody(mesh, referenceId, scale, position, rotation,
                                          0,    // scaledBoxTranslation
                                          0,    // boxRotation
                                          true, // raycasting
                                          placeable);
    }

    void PhysicsSystem::removeObject(const std::string& name)
    {
        mEngine->removeRigidBody(name);
        mEngine->deleteRigidBody(name);
    }

    void PhysicsSystem::addHeightField(float* heights, int x, int y, float yoffset,
                                       float triSize, float sqrtVerts)
    {
        mEngine->addHeightField(heights, x, y, yoffset, triSize, sqrtVerts);
    }

    void PhysicsSystem::removeHeightField(int x, int y)
    {
        mEngine->removeHeightField(x, y);
    }

    std::pair<std::string, Ogre::Vector3> PhysicsSystem::castRay(float mouseX, float mouseY,
                            Ogre::Vector3* normal, std::string* hit, Ogre::Camera *camera)
    {
        if(!mSceneMgr || !camera || !camera->getViewport())
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

        Ogre::Vector3 norm;
        std::pair<std::string, float> result =
                                mEngine->rayTest(_from, _to, !ignoreObjects, ignoreHeightMap, &norm);

        if(result.first == "")
            return std::make_pair("", Ogre::Vector3(0,0,0));
        else
            return std::make_pair(result.first, ray.getPoint(farClipDist*result.second));
    }

    std::string PhysicsSystem::referenceToSceneNode(std::string reference)
    {
        return mRefToSceneNode[reference];
    }

    void PhysicsSystem::setSceneManager(Ogre::SceneManager *sceneMgr)
    {
        mSceneMgr = sceneMgr;
        mEngine->setSceneManager(sceneMgr); // needed for toggleDebugRendering()
    }

    void PhysicsSystem::toggleDebugRendering()
    {
        if(!mSceneMgr)
            return; // FIXME: maybe this should be an exception

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
