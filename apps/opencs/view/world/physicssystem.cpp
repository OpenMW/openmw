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

    // OpenMW                         | OpenCS
    // Ptr                            | ?
    // ptr.getClass().getModel(ptr)   | ? // see Object::update()
    // ptr.getRefData().getBaseNode() | ?
    // ptr.getCellRef().getScale()    | ?
    //
    // getModel() returns the mesh; each class has its own implementation
    //
    //void PhysicsSystem::addObject (const Ptr& ptr, bool placeable)
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
#if 0
        std::string mesh = ptr.getClass().getModel(ptr);
        Ogre::SceneNode* node = ptr.getRefData().getBaseNode();
        handleToMesh[node->getName()] = mesh;
        mEngine->createAndAdjustRigidBody(
            mesh, node->getName(), ptr.getCellRef().getScale(), node->getPosition(), node->getOrientation(), 0, 0, false, placeable);
        mEngine->createAndAdjustRigidBody(
            mesh, node->getName(), ptr.getCellRef().getScale(), node->getPosition(), node->getOrientation(), 0, 0, true, placeable);
#endif
        mEngine->createAndAdjustRigidBody(mesh, name, scale, position, rotation,
                                          0, 0, false, placeable);
        mEngine->createAndAdjustRigidBody(mesh, name, scale, position, rotation,
                                          0, 0, true, placeable);
    }

    void PhysicsSystem::toggleDebugRendering()
    {
        //mEngine->toggleDebugRendering();
        mEngine->setDebugRenderingMode(1);
    }
}
