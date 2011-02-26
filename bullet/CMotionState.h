#ifndef OENGINE_CMOTIONSTATE_H
#define OENGINE_CMOTIONSTATE_H

#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <string>

namespace OEngine {
namespace Physic
{
	class PhysicEngine;

	/**
	*A CMotionState is associated with a single RigidBody.
	*When the RigidBody is moved by bullet, bullet will call the function setWorldTransform.
	*for more info, see the bullet Wiki at btMotionState.
	*/
	class CMotionState:public btMotionState
	{
	public:

		CMotionState(PhysicEngine* eng,std::string name);

		/**
		*Return the position of the RigidBody.
		*/
		virtual void getWorldTransform(btTransform &worldTrans) const;

		/**
		*Function called by bullet when the RigidBody is moved.
		*It add an event to the EventList of the PhysicEngine class.
		*/
		virtual void setWorldTransform(const btTransform &worldTrans);

	protected:
		PhysicEngine* pEng;
		btTransform tr;
		bool isNPC;
		bool isPC;

		std::string pName;
	};

	struct PhysicEvent
	{
		bool isNPC;
		bool isPC;
		btTransform newTransform;
		std::string RigidBodyName;
	};

}}
#endif
