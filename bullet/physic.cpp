#include "physic.hpp"
#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>
#include <components/nifbullet/bullet_nif_loader.hpp>
//#include <apps\openmw\mwworld\world.hpp>
#include "CMotionState.h"
#include "OgreRoot.h"
#include "btKinematicCharacterController.h"
#include "BtOgrePG.h"
#include "BtOgreGP.h"
#include "BtOgreExtras.h"

#define BIT(x) (1<<(x))

namespace OEngine {
namespace Physic
{
    enum collisiontypes {
        COL_NOTHING = 0, //<Collide with nothing
        COL_WORLD = BIT(0), //<Collide with world objects
        COL_ACTOR_INTERNAL = BIT(1), //<Collide internal capsule
        COL_ACTOR_EXTERNAL = BIT(2) //<collide with external capsule
    };

    PhysicActor::PhysicActor(std::string name)
    {
        mName = name;

        // The capsule is at the origin
        btTransform transform;
        transform.setIdentity();

        // External capsule
        externalGhostObject = new PairCachingGhostObject(name);
        externalGhostObject->setWorldTransform( transform );

        btScalar externalCapsuleHeight = 120;
        btScalar externalCapsuleWidth = 19;

        externalCollisionShape = new btCapsuleShapeZ( externalCapsuleWidth,  externalCapsuleHeight );
        externalCollisionShape->setMargin( 0.1 );

        externalGhostObject->setCollisionShape( externalCollisionShape );
        externalGhostObject->setCollisionFlags( btCollisionObject::CF_CHARACTER_OBJECT );

        // Internal capsule
        internalGhostObject = new PairCachingGhostObject(name);
        internalGhostObject->setWorldTransform( transform );
        //internalGhostObject->getBroadphaseHandle()->s
        btScalar internalCapsuleHeight =  110;
        btScalar internalCapsuleWidth =  17;

        internalCollisionShape = new btCapsuleShapeZ( internalCapsuleWidth, internalCapsuleHeight );
        internalCollisionShape->setMargin( 0.1 );

        internalGhostObject->setCollisionShape( internalCollisionShape );
        internalGhostObject->setCollisionFlags( btCollisionObject::CF_CHARACTER_OBJECT );

        mCharacter = new btKinematicCharacterController( externalGhostObject,internalGhostObject,btScalar( 40 ),1,4,20,9.8,0.2 );
        mCharacter->setUpAxis(btKinematicCharacterController::Z_AXIS);
        mCharacter->setUseGhostSweepTest(false);

        mCharacter->mCollision = false;
        setGravity(0);

        mTranslation = btVector3(0,0,70);
    }

    PhysicActor::~PhysicActor()
    {
        delete mCharacter;
        delete internalGhostObject;
        delete internalCollisionShape;
        delete externalGhostObject;
        delete externalCollisionShape;
    }

    void PhysicActor::setGravity(float gravity)
    {
        mCharacter->setGravity(gravity);
        //mCharacter->
    }

    void PhysicActor::enableCollisions(bool collision)
    {
        mCharacter->mCollision = collision;
    }

    void PhysicActor::setVerticalVelocity(float z)
    {
        mCharacter->setVerticalVelocity(z);
    }

    bool PhysicActor::getCollisionMode()
    {
        return mCharacter->mCollision;
    }

    void PhysicActor::setWalkDirection(const btVector3& mvt)
    {
        mCharacter->setWalkDirection( mvt );
    }

    void PhysicActor::Rotate(const btQuaternion& quat)
    {
        externalGhostObject->getWorldTransform().setRotation( externalGhostObject->getWorldTransform().getRotation() * quat );
        internalGhostObject->getWorldTransform().setRotation( internalGhostObject->getWorldTransform().getRotation() * quat );
    }

    void PhysicActor::setRotation(const btQuaternion& quat)
    {
        externalGhostObject->getWorldTransform().setRotation( quat );
        internalGhostObject->getWorldTransform().setRotation( quat );
    }

    btVector3 PhysicActor::getPosition(void)
    {
        return internalGhostObject->getWorldTransform().getOrigin() -mTranslation;
    }

    btQuaternion PhysicActor::getRotation(void)
    {
        return internalGhostObject->getWorldTransform().getRotation();
    }

    void PhysicActor::setPosition(const btVector3& pos)
    {
        internalGhostObject->getWorldTransform().setOrigin(pos+mTranslation);
        externalGhostObject->getWorldTransform().setOrigin(pos+mTranslation);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    RigidBody::RigidBody(btRigidBody::btRigidBodyConstructionInfo& CI,std::string name)
        :btRigidBody(CI),mName(name)
    {

    };



    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////



    PhysicEngine::PhysicEngine(BulletShapeLoader* shapeLoader)
    {
        // Set up the collision configuration and dispatcher
        collisionConfiguration = new btDefaultCollisionConfiguration();
        dispatcher = new btCollisionDispatcher(collisionConfiguration);

        // The actual physics solver
        solver = new btSequentialImpulseConstraintSolver;

        //TODO: memory leak?
        btOverlappingPairCache* pairCache = new btSortedOverlappingPairCache();
        //pairCache->setInternalGhostPairCallback( new btGhostPairCallback() );

        broadphase = new btDbvtBroadphase(pairCache);

        // The world.
        dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher,broadphase,solver,collisionConfiguration);
        dynamicsWorld->setGravity(btVector3(0,0,-10));

        if(BulletShapeManager::getSingletonPtr() == NULL)
        {
            new BulletShapeManager();
        }
        //TODO:singleton?
        mShapeLoader = shapeLoader;

        isDebugCreated = false;
    }

    void PhysicEngine::createDebugRendering()
    {
        if(!isDebugCreated)
        {
            Ogre::SceneManagerEnumerator::SceneManagerIterator iter = Ogre::Root::getSingleton().getSceneManagerIterator();
            iter.begin();
            Ogre::SceneManager* scn = iter.getNext();
            Ogre::SceneNode* node = scn->getRootSceneNode()->createChildSceneNode();
            node->pitch(Ogre::Degree(-90));
            mDebugDrawer = new BtOgre::DebugDrawer(node, dynamicsWorld);
            dynamicsWorld->setDebugDrawer(mDebugDrawer);
            isDebugCreated = true;
            dynamicsWorld->debugDrawWorld();
        }
    }

    void PhysicEngine::setDebugRenderingMode(int mode)
    {
        if(!isDebugCreated)
        {
            createDebugRendering();
        }
        mDebugDrawer->setDebugMode(mode);
    }

    PhysicEngine::~PhysicEngine()
    {
        delete dynamicsWorld;
        delete solver;
        delete collisionConfiguration;
        delete dispatcher;
        delete broadphase;
        delete mShapeLoader;
    }

    RigidBody* PhysicEngine::createRigidBody(std::string mesh,std::string name)
    {
        //get the shape from the .nif
        mShapeLoader->load(mesh,"General");
        BulletShapeManager::getSingletonPtr()->load(mesh,"General");
        BulletShapePtr shape = BulletShapeManager::getSingleton().getByName(mesh,"General");

        //create the motionState
        CMotionState* newMotionState = new CMotionState(this,name);

        //create the real body
        btRigidBody::btRigidBodyConstructionInfo CI = btRigidBody::btRigidBodyConstructionInfo(0,newMotionState,shape->Shape);
        RigidBody* body = new RigidBody(CI,name);
        body->collide = shape->collide;
        return body;
    }

    void PhysicEngine::addRigidBody(RigidBody* body)
    {
        if(body->collide)
        {
            dynamicsWorld->addRigidBody(body,COL_WORLD,COL_WORLD|COL_ACTOR_INTERNAL|COL_ACTOR_EXTERNAL);
        }
        else
        {
            dynamicsWorld->addRigidBody(body,COL_WORLD,COL_NOTHING);
        }
        body->setActivationState(DISABLE_DEACTIVATION);
        RigidBodyMap[body->mName] = body;
    }

    void PhysicEngine::removeRigidBody(std::string name)
    {
        std::map<std::string,RigidBody*>::iterator it = RigidBodyMap.find(name);
        if (it != RigidBodyMap.end() )
        {
            RigidBody* body = it->second;
            if(body != NULL)
            {
                // broadphase->getOverlappingPairCache()->removeOverlappingPairsContainingProxy(body->getBroadphaseProxy(),dispatcher);
                /*std::map<std::string,PhysicActor*>::iterator it2 = PhysicActorMap.begin();
                  for(;it2!=PhysicActorMap.end();it++)
                  {
                  it2->second->internalGhostObject->getOverlappingPairCache()->removeOverlappingPairsContainingProxy(body->getBroadphaseProxy(),dispatcher);
                  it2->second->externalGhostObject->getOverlappingPairCache()->removeOverlappingPairsContainingProxy(body->getBroadphaseProxy(),dispatcher);
                  }*/
                dynamicsWorld->removeRigidBody(RigidBodyMap[name]);
            }
        }
    }

    void PhysicEngine::deleteRigidBody(std::string name)
    {
        std::map<std::string,RigidBody*>::iterator it = RigidBodyMap.find(name);
        if (it != RigidBodyMap.end() )
        {
            RigidBody* body = it->second;
            if(body != NULL)
            {
                delete body;
            }
            RigidBodyMap.erase(it);
        }
    }

    RigidBody* PhysicEngine::getRigidBody(std::string name)
    {
        RigidBody* body = RigidBodyMap[name];
        return body;
    }

    void PhysicEngine::stepSimulation(double deltaT)
    {
        dynamicsWorld->stepSimulation(deltaT,1,1/50.);
        if(isDebugCreated)
        {
            mDebugDrawer->step();
        }
    }

    void PhysicEngine::addCharacter(std::string name)
    {
        PhysicActor* newActor = new PhysicActor(name);
        dynamicsWorld->addCollisionObject( newActor->externalGhostObject, COL_ACTOR_EXTERNAL, COL_WORLD |COL_ACTOR_EXTERNAL );
        dynamicsWorld->addCollisionObject( newActor->internalGhostObject, COL_ACTOR_INTERNAL, COL_WORLD |COL_ACTOR_INTERNAL );
        dynamicsWorld->addAction( newActor->mCharacter );
        PhysicActorMap[name] = newActor;
    }

    void PhysicEngine::removeCharacter(std::string name)
    {
        //std::cout << "remove";
        std::map<std::string,PhysicActor*>::iterator it = PhysicActorMap.find(name);
        if (it != PhysicActorMap.end() )
        {
            PhysicActor* act = it->second;
            if(act != NULL)
            {
                /*broadphase->getOverlappingPairCache()->removeOverlappingPairsContainingProxy(act->externalGhostObject->getBroadphaseHandle(),dispatcher);
                  broadphase->getOverlappingPairCache()->removeOverlappingPairsContainingProxy(act->internalGhostObject->getBroadphaseHandle(),dispatcher);
                  std::map<std::string,PhysicActor*>::iterator it2 = PhysicActorMap.begin();
                  for(;it2!=PhysicActorMap.end();it++)
                  {
                  it->second->internalGhostObject->getOverlappingPairCache()->removeOverlappingPairsContainingProxy(act->externalGhostObject->getBroadphaseHandle(),dispatcher);
                  it->second->externalGhostObject->getOverlappingPairCache()->removeOverlappingPairsContainingProxy(act->externalGhostObject->getBroadphaseHandle(),dispatcher);
                  it->second->internalGhostObject->getOverlappingPairCache()->removeOverlappingPairsContainingProxy(act->internalGhostObject->getBroadphaseHandle(),dispatcher);
                  it->second->externalGhostObject->getOverlappingPairCache()->removeOverlappingPairsContainingProxy(act->internalGhostObject->getBroadphaseHandle(),dispatcher);
                  }*/
                //act->externalGhostObject->
                dynamicsWorld->removeCollisionObject(act->externalGhostObject);
                dynamicsWorld->removeCollisionObject(act->internalGhostObject);
                dynamicsWorld->removeAction(act->mCharacter);
                delete act;
            }
            PhysicActorMap.erase(it);
        }
        //std::cout << "ok";
    }

    PhysicActor* PhysicEngine::getCharacter(std::string name)
    {
        PhysicActor* act = PhysicActorMap[name];
        return act;
    }

    void PhysicEngine::emptyEventLists(void)
    {
    }

    std::pair<std::string,float> PhysicEngine::rayTest(btVector3& from,btVector3& to)
    {
        std::string name = "";
        float d = -1;

        float d1 = 10000.;
        btCollisionWorld::ClosestRayResultCallback resultCallback1(from, to);
        resultCallback1.m_collisionFilterMask = COL_WORLD;
        dynamicsWorld->rayTest(from, to, resultCallback1);
        if (resultCallback1.hasHit())
        {
            name = static_cast<RigidBody&>(*resultCallback1.m_collisionObject).mName;
            d1 = resultCallback1.m_closestHitFraction;
            d = d1;
        }

        btCollisionWorld::ClosestRayResultCallback resultCallback2(from, to);
        resultCallback2.m_collisionFilterMask = COL_ACTOR_INTERNAL|COL_ACTOR_EXTERNAL;
        dynamicsWorld->rayTest(from, to, resultCallback2);
        float d2 = 10000.;
        if (resultCallback2.hasHit())
        {
            d2 = resultCallback1.m_closestHitFraction;
            if(d2<=d1)
            {
                name = static_cast<PairCachingGhostObject&>(*resultCallback2.m_collisionObject).mName;
                d = d2;
            }
        }

        return std::pair<std::string,float>(name,d);
    }
}};
