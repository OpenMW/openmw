#ifndef OENGINE_BULLET_PHYSIC_H
#define OENGINE_BULLET_PHYSIC_H

#include <BulletDynamics\Dynamics\btRigidBody.h>
#include "BulletCollision/CollisionDispatch/btGhostObject.h"
#include <string>
#include <list>
#include <map>

class btRigidBody;
class btBroadphaseInterface;
class btDefaultCollisionConfiguration;
class btSequentialImpulseConstraintSolver;
class btCollisionDispatcher;
class btDiscreteDynamicsWorld;
class btKinematicCharacterController;

namespace BtOgre
{
	class DebugDrawer;
}

class BulletShapeManager;
class ManualBulletShapeLoader;

namespace MWWorld
{
	class World;
}

namespace OEngine {
namespace Physic
{
	class CMotionState;
	struct PhysicEvent;

	/**
	*A physic Actor use a modifed KinematicCharacterController taken in the bullet forum.
	*/
	class PhysicActor
	{
	public:
		PhysicActor(std::string name);

		~PhysicActor();

		/**
		*This function set the walkDirection. This is not relative to the actor orientation.
		*I think it's also needed to take time into account. A typical call should look like this:
		*setWalkDirection( mvt * orientation * dt)
		*/
		void setWalkDirection(btVector3& mvt);

		void Rotate(btQuaternion& quat);

		void setRotation(btQuaternion& quat);

		btVector3 getPosition(void);

		btQuaternion getRotation(void);

		void setPosition(btVector3& pos);

		btKinematicCharacterController* mCharacter;

		btPairCachingGhostObject* internalGhostObject;
		btCollisionShape* internalCollisionShape;

		btPairCachingGhostObject* externalGhostObject;
		btCollisionShape* externalCollisionShape;

		std::string mName;
	};

	/**
	*This class is just an extension of normal btRigidBody in order to add extra info.
	*When bullet give back a btRigidBody, you can just do a static_cast to RigidBody,
	*so one never should use btRigidBody directly!
	*/
	class RigidBody: public btRigidBody
	{
	public:
		RigidBody(btRigidBody::btRigidBodyConstructionInfo& CI,std::string name);
		std::string mName;
	};

	/**
	*The PhysicEngine class contain everything which is needed for Physic.
	*It's needed that Ogre Resources are set up before the PhysicEngine is created.
	*Note:deleting it WILL NOT delete the RigidBody!
	*TODO:unload unused resources?
	*/
	class PhysicEngine
	{
	public:
		PhysicEngine();
		~PhysicEngine();

		/**
		*create a RigidBody.It does not add it to the simulation, but it does add it to the rigidBody Map,
		*so you can get it with the getRigidBody function.
		*/
		RigidBody* createRigidBody(std::string mesh,std::string name);

		/**
		*Add a RigidBody to the simulation
		*/
		void addRigidBody(RigidBody* body);

		/**
		*Remove a RigidBody from the simulation. It does not delete it, and does not remove it from the RigidBodyMap.
		*/
		void removeRigidBody(std::string name);

		/**
		*delete a RigidBody, and remove it from RigidBodyMap.
		*/
		void deleteRigidBody(std::string name);

		/**
		*Return a pointer to a given rigid body.
		*TODO:check if exist
		*/
		RigidBody* getRigidBody(std::string name);

		/**
		*Create and add a character to the scene, and add it to the ActorMap.
		*/
		void addCharacter(std::string name);

		/**
		*Remove a character from the scene. TODO:delete it! for now, a small memory leak^^ done?
		*/
		void removeCharacter(std::string name);

		/**
		*return a pointer to a character
		*TODO:check if the actor exist...
		*/
		PhysicActor* getCharacter(std::string name);

		/**
		*This step the simulation of a given time.
		*/
		void stepSimulation(double deltaT);

		/**
		*Empty events lists
		*/
		void emptyEventLists(void);

		/**
		*Create a debug rendering. It is called by setDebgRenderingMode if it's not created yet.
		*Important Note: this will crash if the Render is not yet initialise!
		*/
		void createDebugRendering();

		/**
		*Set the debug rendering mode. 0 to turn it off.
		*Important Note: this will crash if the Render is not yet initialise!
		*/
		void setDebugRenderingMode(int mode);

		//event list of non player object
		std::list<PhysicEvent> NPEventList;

		//event list affecting the player
		std::list<PhysicEvent> PEventList;

		//Bullet Stuff
		btBroadphaseInterface* broadphase;
		btDefaultCollisionConfiguration* collisionConfiguration;
		btSequentialImpulseConstraintSolver* solver;
		btCollisionDispatcher* dispatcher;
		btDiscreteDynamicsWorld* dynamicsWorld;

		//the NIF file loader.
		ManualBulletShapeLoader* ShapeLoader;

		std::map<std::string,RigidBody*> RigidBodyMap;
		std::map<std::string,PhysicActor*> PhysicActorMap;

		//debug rendering
		BtOgre::DebugDrawer* mDebugDrawer;
		bool isDebugCreated;
	};

}}

#endif