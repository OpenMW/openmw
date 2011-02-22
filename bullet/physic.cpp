#include "physic.hpp"
#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>
#include <components\nifbullet\bullet_nif_loader.hpp>
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
		externalGhostObject = new btPairCachingGhostObject();
		externalGhostObject->setWorldTransform( transform );

		btScalar externalCapsuleHeight = 50;
		btScalar externalCapsuleWidth = 20;

		externalCollisionShape = new btCapsuleShapeZ( externalCapsuleWidth,  externalCapsuleHeight );
		externalCollisionShape->setMargin( 1 );

		externalGhostObject->setCollisionShape( externalCollisionShape );
		externalGhostObject->setCollisionFlags( btCollisionObject::CF_CHARACTER_OBJECT );

		// Internal capsule
		internalGhostObject = new btPairCachingGhostObject();
		internalGhostObject->setWorldTransform( transform );
		//internalGhostObject->getBroadphaseHandle()->s
		btScalar internalCapsuleHeight =  20;
		btScalar internalCapsuleWidth =  5;

		internalCollisionShape = new btCapsuleShapeZ( internalCapsuleWidth, internalCapsuleHeight );
		internalCollisionShape->setMargin( 1 );

		internalGhostObject->setCollisionShape( internalCollisionShape );
		internalGhostObject->setCollisionFlags( btCollisionObject::CF_CHARACTER_OBJECT );

		mCharacter = new btKinematicCharacterController( externalGhostObject,internalGhostObject,btScalar( 0.4 ),1,0 );
		mCharacter->setUpAxis(btKinematicCharacterController::UpAxis::Z_AXIS);
	}

	PhysicActor::~PhysicActor()
	{
		delete mCharacter;
		delete internalGhostObject;
		delete internalCollisionShape;
		delete externalGhostObject;
		delete externalCollisionShape;
	}

	void PhysicActor::setWalkDirection(btVector3& mvt)
	{
		mCharacter->setWalkDirection( mvt );
	}

	void PhysicActor::Rotate(btQuaternion& quat)
	{
		externalGhostObject->getWorldTransform().setRotation( externalGhostObject->getWorldTransform().getRotation() * quat );
		internalGhostObject->getWorldTransform().setRotation( internalGhostObject->getWorldTransform().getRotation() * quat );
	}

	void PhysicActor::setRotation(btQuaternion& quat)
	{
		externalGhostObject->getWorldTransform().setRotation( quat );
		internalGhostObject->getWorldTransform().setRotation( quat );
	}

	btVector3 PhysicActor::getPosition(void)
	{
		return internalGhostObject->getWorldTransform().getOrigin();
	}

	btQuaternion PhysicActor::getRotation(void)
	{
		return internalGhostObject->getWorldTransform().getRotation();
	}

	void PhysicActor::setPosition(btVector3& pos)
	{
		internalGhostObject->getWorldTransform().setOrigin(pos);
		externalGhostObject->getWorldTransform().setOrigin(pos);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	RigidBody::RigidBody(btRigidBody::btRigidBodyConstructionInfo& CI,std::string name)
		:btRigidBody(CI),mName(name)
	{

	};



	///////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////



	PhysicEngine::PhysicEngine()
	{
		// Set up the collision configuration and dispatcher
		collisionConfiguration = new btDefaultCollisionConfiguration();
		dispatcher = new btCollisionDispatcher(collisionConfiguration);

		// The actual physics solver
		solver = new btSequentialImpulseConstraintSolver;

		//TODO: memory leak?
		btOverlappingPairCache* pairCache = new btSortedOverlappingPairCache();
		pairCache->setInternalGhostPairCallback( new btGhostPairCallback() );

	    broadphase = new btDbvtBroadphase(pairCache);

		// The world.
		dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher,broadphase,solver,collisionConfiguration);
		dynamicsWorld->setGravity(btVector3(0,0,-10));

		if(BulletShapeManager::getSingletonPtr() == NULL)
		{
			new BulletShapeManager();
		}
		//TODO:singleton?
		ShapeLoader = new ManualBulletShapeLoader();

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
		delete ShapeLoader;
	}

	RigidBody* PhysicEngine::createRigidBody(std::string mesh,std::string name)
	{
		//get the shape from the .nif
		ShapeLoader->load(mesh,"General");
		BulletShapeManager::getSingletonPtr()->load(mesh,"General");
		BulletShapePtr shape = BulletShapeManager::getSingleton().getByName(mesh,"General");

		//create the motionState
		CMotionState* newMotionState = new CMotionState(this,name);

		//create the real body
		btRigidBody::btRigidBodyConstructionInfo CI = btRigidBody::btRigidBodyConstructionInfo(0,newMotionState,shape->Shape);
		RigidBody* body = new RigidBody(CI,name);

		return body;
	}

	void PhysicEngine::addRigidBody(RigidBody* body)
	{
		dynamicsWorld->addRigidBody(body,COL_WORLD,COL_WORLD|COL_ACTOR_INTERNAL|COL_ACTOR_EXTERNAL);
		body->setActivationState(DISABLE_DEACTIVATION);
		RigidBodyMap[body->mName] = body;
	}

	void PhysicEngine::removeRigidBody(std::string name)
	{
		RigidBody* body = RigidBodyMap[name];
		if(body != NULL)
		{
			dynamicsWorld->removeRigidBody(RigidBodyMap[name]);
		}
	}

	void PhysicEngine::deleteRigidBody(std::string name)
	{
		RigidBody* body = RigidBodyMap[name];
		if(body != NULL)
		{
			delete body;
			RigidBodyMap[name] = NULL;
		}
	}
	
	RigidBody* PhysicEngine::getRigidBody(std::string name)
	{
		RigidBody* body = RigidBodyMap[name];
		return body;
	}

	void PhysicEngine::stepSimulation(double deltaT)
	{
		dynamicsWorld->stepSimulation(deltaT,1,1/30.);
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
		PhysicActor* act = PhysicActorMap[name];
		if(act != NULL)
		{
			dynamicsWorld->removeCollisionObject(act->externalGhostObject);
			dynamicsWorld->removeCollisionObject(act->internalGhostObject);
			dynamicsWorld->removeAction(act->mCharacter);
			delete act;
			PhysicActorMap[name] = NULL;
		}
	}

	PhysicActor* PhysicEngine::getCharacter(std::string name)
	{
		PhysicActor* act = PhysicActorMap[name];
		return act;
	}

	void PhysicEngine::emptyEventLists(void)
	{
	}
}};