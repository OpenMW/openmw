
#include "trace.h"

#include <map>

#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>

#include "physic.hpp"


enum traceWorldType
{
    collisionWorldTrace = 1,
    pickWorldTrace = 2,
    bothWorldTrace = collisionWorldTrace | pickWorldTrace
};

enum collaborativePhysicsType
{
    No_Physics = 0,     // Both are empty (example: statics you can walk through, like tall grass)
    Only_Collision = 1, // This object only has collision physics but no pickup physics (example: statics)
    Only_Pickup = 2,    // This object only has pickup physics but no collision physics (example: items dropped on the ground)
    Both_Physics = 3    // This object has both kinds of physics (example: activators)
};

void newtrace(traceResults *results, const Ogre::Vector3& start, const Ogre::Vector3& end, const Ogre::Vector3& BBHalfExtents, bool isInterior, OEngine::Physic::PhysicEngine *enginePass)  //Traceobj was a Aedra Object
{
    const btVector3 btstart(start.x, start.y, start.z + BBHalfExtents.z);
    const btVector3 btend(end.x, end.y, end.z + BBHalfExtents.z);
    const btQuaternion btrot(0.0f, 0.0f, 0.0f);   //y, x, z

    const btBoxShape newshape(btVector3(BBHalfExtents.x, BBHalfExtents.y, BBHalfExtents.z));
    //const btCapsuleShapeZ newshape(BBHalfExtents.x, BBHalfExtents.z * 2 - BBHalfExtents.x * 2);
    const btTransform from(btrot, btstart);
    const btTransform to(btrot, btend);

    btCollisionWorld::ClosestConvexResultCallback newTraceCallback(btstart, btend);
    newTraceCallback.m_collisionFilterMask = Only_Collision;

    enginePass->dynamicsWorld->convexSweepTest(&newshape, from, to, newTraceCallback);

    // Copy the hit data over to our trace results struct:
    if(newTraceCallback.hasHit())
    {
        const btVector3& tracehitnormal = newTraceCallback.m_hitNormalWorld;
        results->fraction = newTraceCallback.m_closestHitFraction;
        results->planenormal = Ogre::Vector3(tracehitnormal.x(), tracehitnormal.y(), tracehitnormal.z());
        results->endpos = (end-start)*results->fraction + start;
    }
    else
    {
        results->endpos = end;
        results->planenormal = Ogre::Vector3(0.0f, 0.0f, 1.0f);
        results->fraction = 1.0f;
    }
}
