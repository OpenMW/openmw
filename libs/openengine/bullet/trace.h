#ifndef OENGINE_BULLET_TRACE_H
#define OENGINE_BULLET_TRACE_H


#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>
#include <components/nifbullet/bullet_nif_loader.hpp>
#include <openengine/bullet/pmove.h>
#include <openengine/bullet/physic.hpp>



enum traceWorldType
{
	collisionWorldTrace = 1,
	pickWorldTrace = 2,
	bothWorldTrace = collisionWorldTrace | pickWorldTrace
};

enum collaborativePhysicsType
{
	No_Physics = 0, // Both are empty (example: statics you can walk through, like tall grass)
	Only_Collision = 1, // This object only has collision physics but no pickup physics (example: statics)
	Only_Pickup = 2, // This object only has pickup physics but no collision physics (example: items dropped on the ground)
	Both_Physics = 3 // This object has both kinds of physics (example: activators)
};

struct NewPhysTraceResults
{
	Ogre::Vector3 endPos;
	Ogre::Vector3 hitNormal;
	float fraction;
	bool startSolid;
	//const Object* hitObj;
};
struct traceResults
{
	Ogre::Vector3 endpos;
	Ogre::Vector3 planenormal;

	float fraction;

	int surfaceFlags;
	int contents;
	int entityNum;

	bool allsolid;
	bool startsolid;
};



template <const traceWorldType traceType>
const bool NewPhysicsTrace(NewPhysTraceResults* const out, const Ogre::Vector3& start, const Ogre::Vector3& end, const Ogre::Vector3& BBExtents, const Ogre::Vector3& rotation, bool isInterior, OEngine::Physic::PhysicEngine* enginePass);
//template const bool NewPhysicsTrace<collisionWorldTrace>(NewPhysTraceResults* const out, const Ogre::Vector3& start, const Ogre::Vector3& end, const Ogre::Vector3& BBExtents, const Ogre::Vector3& rotation, bool isInterior, OEngine::Physic::PhysicEngine* enginePass);
//template const bool NewPhysicsTrace<pickWorldTrace>(NewPhysTraceResults* const out, const Ogre::Vector3& start, const Ogre::Vector3& end, const Ogre::Vector3& BBExtents, const Ogre::Vector3& rotation, bool isInterior, OEngine::Physic::PhysicEngine* enginePass);

void newtrace(traceResults* const results, const Ogre::Vector3& start, const Ogre::Vector3& end, const Ogre::Vector3& BBExtents, const float rotation, bool isInterior, OEngine::Physic::PhysicEngine* enginePass);


#endif
