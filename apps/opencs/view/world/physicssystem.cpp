#include "physicssystem.hpp"

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
                                  Ogre::Vector3* scaledBoxTranslation,
                                  Ogre::Quaternion* boxRotation,
                                  bool raycasting,
                                  bool placeable)
    {
        //handleToMesh[node->getName()] = mesh;
        mEngine->createAndAdjustRigidBody(mesh, name, scale, position, rotation,
                                          scaledBoxTranslation, boxRotation, raycasting, placeable);
    }

    void PhysicsSystem::toggleDebugRendering()
    {
        mEngine->toggleDebugRendering();
        //mEngine->setDebugRenderingMode(1);
    }
}
