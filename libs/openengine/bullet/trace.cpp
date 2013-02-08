
#include "trace.h"

#include <map>

#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>

#include "physic.hpp"
#include "pmove.h"


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

struct NewPhysTraceResults
{
    Ogre::Vector3 endPos;
    Ogre::Vector3 hitNormal;
    float fraction;
    bool startSolid;
    //const Object* hitObj;
};

template <const traceWorldType traceType>
static const bool NewPhysicsTrace(NewPhysTraceResults* const out, const Ogre::Vector3& start, const Ogre::Vector3& end,
                                  const Ogre::Vector3& BBHalfExtents, const Ogre::Vector3& rotation, bool isInterior,
                                  OEngine::Physic::PhysicEngine* enginePass)
{
    const btVector3 btstart(start.x, start.y, start.z + BBHalfExtents.z);
    const btVector3 btend(end.x, end.y, end.z + BBHalfExtents.z);
    const btQuaternion btrot(rotation.y, rotation.x, rotation.z);   //y, x, z

    const btBoxShape newshape(btVector3(BBHalfExtents.x, BBHalfExtents.y, BBHalfExtents.z));
    //const btCapsuleShapeZ newshape(BBHalfExtents.x, BBHalfExtents.z * 2 - BBHalfExtents.x * 2);
    const btTransform from(btrot, btstart);
    const btTransform to(btrot, btend);

    btCollisionWorld::ClosestConvexResultCallback newTraceCallback(btstart, btend);

    newTraceCallback.m_collisionFilterMask = (traceType == collisionWorldTrace) ? Only_Collision : Only_Pickup;

    enginePass->dynamicsWorld->convexSweepTest(&newshape, from, to, newTraceCallback);

    // Copy the hit data over to our trace results struct:
    out->fraction = newTraceCallback.m_closestHitFraction;

    const btVector3& tracehitnormal = newTraceCallback.m_hitNormalWorld;
    out->hitNormal.x = tracehitnormal.x();
    out->hitNormal.y = tracehitnormal.y();
    out->hitNormal.z = tracehitnormal.z();

    const btVector3& tracehitpos = newTraceCallback.m_hitPointWorld;
    out->endPos.x = tracehitpos.x();
    out->endPos.y = tracehitpos.y();
    out->endPos.z = tracehitpos.z();

    // StartSolid test:
    {
        out->startSolid = false;
        if(isInterior)
        {
            // If inside and out of the tree, we're solid
            btVector3 aabbMin, aabbMax;
            enginePass->broadphase->getBroadphaseAabb(aabbMin, aabbMax);
            btVector3 point(start.x, start.y, start.z);
            if(!TestPointAgainstAabb2(aabbMin, aabbMax, point))
            {
                //We're solid
                //THIS NEEDS TO BE TURNED OFF IF WE WANT FALLING IN EXTERIORS TO WORK CORRECTLY!!!!!!!
                //out->startSolid = true;
            }
        }
    }

    return newTraceCallback.hasHit();
}


void newtrace(traceResults* const results, const Ogre::Vector3& start, const Ogre::Vector3& end, const Ogre::Vector3& BBHalfExtents, const float rotation, bool isInterior, OEngine::Physic::PhysicEngine* enginePass)  //Traceobj was a Aedra Object
{
    NewPhysTraceResults out;
    const bool hasHit = NewPhysicsTrace<collisionWorldTrace>(&out, start, end, BBHalfExtents,
                                                             Ogre::Vector3(0.0f, 0.0f, 0.0f),
                                                             isInterior, enginePass);
    if (out.fraction < 0.001f)
        results->startsolid = true;
    else
        results->startsolid = false;
    results->allsolid = out.startSolid;

    if(!hasHit)
    {
        results->endpos = end;
        results->planenormal = Ogre::Vector3(0.0f, 0.0f, 1.0f);
        results->entityNum = ENTITYNUM_NONE;
        results->fraction = 1.0f;
    }
    else
    {
        results->fraction = out.fraction;
        results->planenormal = out.hitNormal;
        results->endpos = (end-start)*results->fraction + start;
        results->entityNum = ENTITYNUM_WORLD;
    }
}
