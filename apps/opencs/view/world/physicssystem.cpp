#include "physicssystem.hpp"

#include <OgreRay.h>
#include <OgreCamera.h>
#include <OgreRoot.h> // FIXME: renderOneFrame

#include <openengine/bullet/physic.hpp>
//#include <openengine/bullet/BtOgreExtras.h>
//#include <openengine/ogre/renderer.hpp>

#include <components/nifbullet/bulletnifloader.hpp>

namespace CSVWorld
{
    PhysicsSystem::PhysicsSystem(Ogre::SceneManager *sceneMgr)
    {
        // Create physics. shapeLoader is deleted by the physic engine
        NifBullet::ManualBulletShapeLoader* shapeLoader = new NifBullet::ManualBulletShapeLoader();
        mEngine = new OEngine::Physic::PhysicEngine(shapeLoader);

        mEngine->setSceneManager(sceneMgr); // needed for toggleDebugRendering()
    }

    PhysicsSystem::~PhysicsSystem()
    {
        delete mEngine;
    }

    void PhysicsSystem::addObject(const std::string &mesh,
                                  const std::string &name,
                                  float scale,
                                  const Ogre::Vector3 &position,
                                  const Ogre::Quaternion &rotation,
                                  //Ogre::Vector3* scaledBoxTranslation,
                                  //Ogre::Quaternion* boxRotation,
                                  //bool raycasting,
                                  bool placeable)
    {
        //handleToMesh[node->getName()] = mesh;

        mEngine->createAndAdjustRigidBody(mesh, name, scale, position, rotation,
                                          0, 0, true, placeable);
    }

    void PhysicsSystem::toggleDebugRendering()
    {
        mEngine->toggleDebugRendering();
        mEngine->stepSimulation(0.0167); // FIXME: DebugDrawer::step() not accessible
        Ogre::Root::getSingleton().renderOneFrame();  // FIXME: temporary workaround for immediate visual feedback
    }

    /*std::pair<bool, Ogre::Vector3>*/ void PhysicsSystem::castRay(float mouseX, float mouseY,
                                        Ogre::Vector3* normal, std::string* hit, Ogre::Camera *camera)
    {
        Ogre::Ray ray = camera->getCameraToViewportRay(
            mouseX,
            mouseY);
        Ogre::Vector3 from = ray.getOrigin();
        Ogre::Vector3 to = ray.getPoint(200000);
        //Ogre::Vector3 to = ray.getDirection();

        btVector3 _from, _to;
        _from = btVector3(from.x, from.y, from.z);
        _to = btVector3(to.x, to.y, to.z);

        bool raycastingObjectOnly = true;
        bool ignoreHeightMap = false;
        Ogre::Vector3 norm;
        std::pair<std::string, float> result = mEngine->rayTest(_from, _to, raycastingObjectOnly, ignoreHeightMap, &norm);

        if (result.first == "")
            //return std::make_pair(false, Ogre::Vector3());
            std::cout << "no hit" << std::endl;
        else
        {
            std::cout << "hit " << result.first
                            + " result " + std::to_string(result.second) << std::endl;
            std::cout << "normal " + std::to_string(norm.x)
                            + ", " + std::to_string(norm.y)
                            + ", " + std::to_string(norm.z) << std::endl;
            std::cout << "hit pos" + std::to_string(ray.getPoint(200000*result.second).x)
                            + ", " + std::to_string(ray.getPoint(200000*result.second).y)
                            + ", " + std::to_string(ray.getPoint(200000*result.second).z) << std::endl;
        }
    }
}
